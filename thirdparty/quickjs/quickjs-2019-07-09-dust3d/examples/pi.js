/*
 * PI computation in Javascript using the QuickJS bignum extensions
 */
"use strict";
"use bigint";

/* compute PI with a precision of 'prec' bits */
function calc_pi(prec) {
    const CHUD_A = 13591409;
    const CHUD_B = 545140134;
    const CHUD_C = 640320;
    const CHUD_C3 = 10939058860032000; /* C^3/24 */
    const CHUD_BITS_PER_TERM = 47.11041313821584202247; /* log2(C/12)*3 */
    
    /* return [P, Q, G] */
    function chud_bs(a, b, need_G) {
        var c, P, Q, G, P1, Q1, G1, P2, Q2, G2;
        if (a == (b - 1)) {
            G = (2 * b - 1) * (6 * b - 1) * (6 * b - 5);
            P = BigFloat(G * (CHUD_B * b + CHUD_A));
            if (b & 1)
                P = -P;
            G = BigFloat(G);
            Q = BigFloat(b * b * b * CHUD_C3);
        } else {
            c = (a + b) >> 1;
            [P1, Q1, G1] = chud_bs(a, c, true);
            [P2, Q2, G2] = chud_bs(c, b, need_G);
            P = P1 * Q2 + P2 * G1;
            Q = Q1 * Q2;
            if (need_G)
                G = G1 * G2;
            else
                G = 0;
        }
        return [P, Q, G];
    }

    var n, P, Q, G;
    /* number of serie terms */
    n = Math.ceil(BigFloatEnv.prec / CHUD_BITS_PER_TERM) + 10;
    [P, Q, G] = chud_bs(0, n, false);
    Q = Q / (P + Q * CHUD_A);
    G = (CHUD_C / 12) * BigFloat.sqrt(CHUD_C);
    return Q * G;
}

(function() {
    var r, n_digits, n_bits;
    if (typeof scriptArgs != "undefined") {
        if (scriptArgs.length < 2) {
            print("usage: pi n_digits");
            return;
        }
        n_digits = scriptArgs[1];
    } else {
        n_digits = 1000;
    }
    n_bits = Math.ceil(n_digits * Math.log2(10));
    /* we add more bits to reduce the probability of bad rounding for
       the last digits */
    BigFloatEnv.setPrec( () => {
        r = calc_pi();
        print(r.toFixed(n_digits, BigFloatEnv.RNDZ));
    }, n_bits + 32);
})();
