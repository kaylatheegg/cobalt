#define HE HI
#define LLO _THERE
#define HELLO "HI THERE"
#define CAT(a,b) a##b
#define XCAT(a,b) CAT(a,b)
#define CALL(fn) fn(HE,LLO)

CAT(HE, LLO) // "HI THERE", because concatenation occurs before normal expansion
XCAT(HE, LLO) // HI_THERE, because the tokens originating from parameters ("HE" and "LLO") are expanded first
CALL(CAT) // "HI THERE", because this evaluates to CAT(a,b)