struct State {
};

{{ GENERATED_CODE }}

void evaluate(Context ctx) {
    auto x = getValue<input_X>(ctx);
    emitValue<output_CEIL>(ctx, ceil(x));
}
