/* nd_size_stub.c: fixture for nd_size_test.sh (S0-4). Stands in for nd_core.c until S1-2. A small loop with one callee and a
 * volatile sink, so the optimiser keeps both. Not game code. */
volatile int nd_size_sink;
__attribute__((noinline)) static int nd_size_callee(int x) { return x * 3 + 1; }
void nd_emit(int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += nd_size_callee(i);
    nd_size_sink = s;
}
