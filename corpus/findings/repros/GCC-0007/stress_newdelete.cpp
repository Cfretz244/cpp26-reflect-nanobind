// Pure heap churn: new/delete in a consteval loop, no strings.
#ifndef N
#define N 500000
#endif
consteval unsigned long churn() {
    unsigned long t = 0;
    for (int i = 0; i < N; ++i) { int* p = new int(i); t += *p; delete p; }
    return t;
}
static_assert(churn() > 0);
