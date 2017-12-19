{{!-- Template for GENERATED_CODE token inside each patch implementation --}}
{{!-- Accepts TPatch context --}}

struct Node {
    State state;
  {{#if usesTimeouts}}
    TimeMs timeoutAt;
  {{/if}}
  {{#each outputs}}
    {{ cppType type }} output_{{ pinKey }};
  {{/each}}

    union {
        struct {
          {{#eachDirtyable outputs}}
            bool isOutputDirty_{{ pinKey }} : 1;
          {{/eachDirtyable}}
            bool isNodeDirty : 1;
        };

        DirtyFlags dirtyFlags;
    };
};

{{#each inputs}}
struct input_{{ pinKey }} { };
{{/each}}
{{#each outputs}}
struct output_{{ pinKey }} { };
{{/each}}

template<typename PinT> struct ValueType { using T = void; };
{{#each inputs}}
template<> struct ValueType<input_{{ pinKey }}> { using T = {{ cppType type }}; };
{{/each}}
{{#each outputs}}
template<> struct ValueType<output_{{ pinKey }}> { using T = {{ cppType type }}; };
{{/each}}


struct ContextObject {
    Node* _node;

  {{#each inputs}}
    {{ cppType type }} _input_{{ pinKey }};
  {{/each}}

  {{#eachDirtyable inputs}}
    bool _isInputDirty_{{ pinKey }};
  {{/eachDirtyable}}
};

using Context = ContextObject*;

template<typename PinT> typename ValueType<PinT>::T getValue(Context ctx) {
    static_assert(always_false<PinT>::value,
            "Invalid pin descriptor. Expected one of:" \
            "{{#each inputs}} input_{{pinKey}}{{/each}}" \
            "{{#each outputs}} output_{{pinKey}}{{/each}}");
}

{{#each inputs}}
template<> {{ cppType type }} getValue<input_{{ pinKey }}>(Context ctx) {
    return ctx->_input_{{ pinKey }};
}
{{/each}}
{{#each outputs}}
template<> {{ cppType type }} getValue<output_{{ pinKey }}>(Context ctx) {
    return ctx->_node->output_{{ pinKey }};
}
{{/each}}

template<typename InputT> bool isInputDirty(Context ctx) {
    static_assert(always_false<InputT>::value,
            "Invalid input descriptor. Expected one of:" \
            "{{#eachDirtyable inputs}} input_{{pinKey}}{{/eachDirtyable}}");
}

{{#eachDirtyable inputs}}
template<> bool isInputDirty<input_{{ pinKey }}>(Context ctx) {
    return ctx->_isInputDirty_{{ pinKey }};
}
{{/eachDirtyable}}

template<typename OutputT> void emitValue(Context ctx, typename ValueType<OutputT>::T val) {
    static_assert(always_false<OutputT>::value,
            "Invalid output descriptor. Expected one of:" \
            "{{#each outputs}} output_{{pinKey}}{{/each}}");
}

{{#each outputs}}
template<> void emitValue<output_{{ pinKey }}>(Context ctx, {{ cppType type }} val) {
    ctx->_node->output_{{ pinKey }} = val;
  {{#if isDirtyable ~}}
    ctx->_node->isOutputDirty_{{ pinKey }} = true;
  {{~/if}}
}
{{/each}}

State* getState(Context ctx) {
    return &ctx->_node->state;
}
