
/*=============================================================================
 *
 *
 * Main loop components
 *
 *
 =============================================================================*/

namespace xod {

// Define/allocate persistent storages (state, timeout, output data) for all nodes
{{#each nodes}}
{{~ mergePins }}
{{#each outputs }}
{{ decltype type value }} node_{{ ../id }}_output_{{ pinKey }} = {{ cppValue type value }};
{{/each}}
{{#unless patch.isConstant}}
{{ ns patch }}::Node node_{{ id }} = {
    { }, // state
  {{#if patch.usesTimeout}}
    0, // timeoutAt
  {{/if}}
  {{#each outputs}}
    node_{{ ../id }}_output_{{ pinKey }},
  {{/each}}
  {{#eachDirtyable outputs}}
    {{ isDirtyOnBoot }}, // {{ pinKey }} dirty
  {{/eachDirtyable}}
    true // node itself dirty
};
{{/unless}}
{{/each}}

void idle() {
    TimeMs now = millis();
    {{#eachNodeUsingTimeouts}}
    detail::checkTriggerTimeout(&node_{{ id }}, now);
    {{/eachNodeUsingTimeouts}}
}

void runTransaction(bool firstRun) {
    g_transactionTime = millis();

    XOD_TRACE_F("Transaction started, t=");
    XOD_TRACE_LN(g_transactionTime);

    // defer-* nodes are always at the very bottom of the graph,
    // so no one will recieve values emitted by them.
    // We must evaluate them before everybody else
    // to give them a chance to emit values.
  {{#eachDeferNode }}
    {
        Storage& node = node_{{ id }};
        if (detail::isTimedOut(&node)) {
            node.isOutputDirty_OUT = true;

            // mark downstream nodes dirty
          {{#each outputs }}
            {{#each to}}
            node_{{ this }}.isNodeDirty = true;
            {{/each}}
          {{/each}}

            // Clear node dirty flag (do not clear output dirieness), so it
            // will evaluate on the regular pass only if it pushed a new value
            // again.
            //
            // We must save dirty output flags, or `isInputDirty` will not work
            // correctly in downstream nodes.
            node.isNodeDirty = false;

            detail::clearTimeout(&node);
        }
    }
  {{/eachDeferNode }}

    // Evaluate all dirty nodes
  {{#eachNonConstantNode}}
    { // {{ ns patch }} #{{ id }}
        if (node_{{ id }}.isNodeDirty) {
            {{ns patch }}::ContextObject ctxObj;
            ctxObj._node = &node_{{ id }};

            // copy data from upstream nodes into context
          {{#eachLinkedInput}}
            {{!--
              // We refer to node_42.output_FOO as data source in case
              // of a regular node and directly use node_42_output_VAL
              // initial value constexpr in case of a constant. It’s
              // because store no Node structures at the global level
            --}}
            ctxObj._input_{{ pinKey }} = node_{{ nodeId }}
                {{~#if patch.isConstant }}_{{else}}.{{/if~}}
                output_{{ fromPinKey }};
          {{/eachLinkedInput}}

          {{#eachLinkedInput}}
            {{!--
              // Constants do not store dirtieness. They are never dirty
              // except the very first run
              //
              // TODO: do not copy upstream dirtieness if the upstream
              // pin is not dirtyable
            --}}
            {{#if isDirtyable}}
            ctxObj._isInputDirty_{{ pinKey }} = {{#if patch.isConstant ~}}
                firstRun{{else}}node_{{ nodeId }}.isOutputDirty_{{ fromPinKey }}{{/if}};
            {{/if}}
          {{/eachLinkedInput}}

            {{ ns patch }}::evaluate(&ctxObj);

            // mark downstream nodes dirty
          {{#each outputs }}
            {{#if isDirtyable ~}}
            {{#each to}}
            node_{{ this }}.isNodeDirty |= node_{{ ../../id }}.isOutputDirty_{{ ../pinKey }};
            {{/each}}
            {{else}}
            {{#each to}}
            node_{{ this }}.isNodeDirty = true;
            {{/each}}
            {{/if}}
          {{/each}}
        }
    }
  {{/eachNonConstantNode}}

    // Clear dirtieness and timeouts for all nodes and pins
  {{#eachNonConstantNode}}
    node_{{ id }}.dirtyFlags = 0;
  {{/eachNonConstantNode}}
  {{#eachNodeUsingTimeouts}}
    detail::clearStaleTimeout(&node_{{ id }});
  {{/eachNodeUsingTimeouts}}

    XOD_TRACE_F("Transaction completed, t=");
    XOD_TRACE_LN(millis());
}

} // namespace xod


// TODO: move to own file
//----------------------------------------------------------------------------
// Entry point
//----------------------------------------------------------------------------
void setup() {
    // FIXME: looks like there is a rounding bug. Waiting for 100ms fights it
    delay(100);
#ifdef XOD_DEBUG
    DEBUG_SERIAL.begin(115200);
#endif
    XOD_TRACE_FLN("\n\nProgram started");

    xod::runTransaction(true);
}

void loop() {
    xod::idle();
    xod::runTransaction(false);
}
