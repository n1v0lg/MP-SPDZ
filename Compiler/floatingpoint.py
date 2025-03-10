from math import log, floor, ceil
from Compiler.instructions import *
import types
import comparison
import program
import util

##
## Helper functions for floating point arithmetic
##


def two_power(n):
    if isinstance(n, int) and n < 31:
        return 2**n
    else:
        max = types.cint(1) << 31
        res = 2**(n%31)
        for i in range(n / 31):
            res *= max
        return res

def shift_two(n, pos):
    if pos < 63:
        return n >> pos
    else:
        res = (n >> (pos%63))
        for i in range(pos / 63):
            res >>= 63
        return res


def maskRing(a, k):
    shift = int(program.Program.prog.options.ring) - k
    r = [types.sint.get_random_bit() for i in range(k)]
    r_prime = types.sint.bit_compose(r)
    c = ((a + r_prime) << shift).reveal() >> shift
    return c, r

def maskField(a, k, kappa):
    r_dprime = types.sint()
    r_prime = types.sint()
    c = types.cint()
    r = [types.sint() for i in range(k)]
    comparison.PRandM(r_dprime, r_prime, r, k, k, kappa)
    asm_open(c, a + two_power(k) * r_dprime + r_prime)# + 2**(k-1))
    return c, r

def EQZ(a, k, kappa):
    if program.Program.prog.options.ring:
        c, r = maskRing(a, k)
    else:
        c, r = maskField(a, k, kappa)
    d = [None]*k
    for i,b in enumerate(bits(c, k)):
        d[i] = b + r[i] - 2*b*r[i]
    return 1 - KOR(d, kappa)

def bits(a,m):
    """ Get the bits of an int """
    if isinstance(a, int):
        res = [None]*m
        for i in range(m):
            res[i] = a & 1
            a >>= 1
    else:
        res = []
        from Compiler.types import regint, cint
        while m > 0:
            aa = regint()
            convmodp(aa, a, bitlength=0)
            res += [cint(x) for x in aa.bit_decompose(min(64, m))]
            m -= 64
            if m > 0:
                aa = cint()
                shrci(aa, a, 64)
                a = aa
    return res

def carry(b, a, compute_p=True):
    """ Carry propogation:
        (p,g) = (p_2, g_2)o(p_1, g_1) -> (p_1 & p_2, g_2 | (p_2 & g_1))
    """
    if compute_p:
        t1 = a[0]*b[0]
    else:
        t1 = None
    t2 = a[1] + a[0]*b[1]
    return (t1, t2)

def or_op(a, b, void=None):
    return a + b - a*b

def mul_op(a, b, void=None):
    return a * b

def PreORC(a, kappa=None, m=None, raw=False):
    k = len(a)
    if k == 1:
        return [a[0]]
    m = m or k
    if isinstance(a[0], types.sgf2n):
        max_k = program.Program.prog.galois_length - 1
    else:
        max_k = int(log(program.Program.prog.P) / log(2)) - kappa
    assert(max_k > 0)
    if k <= max_k:
        p = [None] * m
        if m == k:
            p[0] = a[0]
        if isinstance(a[0], types.sgf2n):
            b = comparison.PreMulC([3 - a[i] for i in range(k)])
            for i in range(m):
                tmp = b[k-1-i]
                if not raw:
                    tmp = tmp.bit_decompose()[0]
                p[m-1-i] = 1 - tmp
        else:
            t = [types.sint() for i in range(m)]
            b = comparison.PreMulC([a[i] + 1 for i in range(k)])
            for i in range(m):
                comparison.Mod2(t[i], b[k-1-i], k, kappa, False)
                p[m-1-i] = 1 - t[i]
        return p
    else:
        # not constant-round anymore
        s = [PreORC(a[i:i+max_k], kappa, raw=raw) for i in range(0,k,max_k)]
        t = PreORC([si[-1] for si in s[:-1]], kappa, raw=raw)
        return sum(([or_op(x, y) for x in si] for si,y in zip(s[1:],t)), s[0])

def PreOpL(op, items):
    """
    Uses algorithm from SecureSCM WP9 deliverable.
    
    op must be a binary function that outputs a new register
    """
    k = len(items)
    logk = int(ceil(log(k,2)))
    kmax = 2**logk
    output = list(items)
    for i in range(logk):
        for j in range(kmax/(2**(i+1))):
            y = two_power(i) + j*two_power(i+1) - 1
            for z in range(1, 2**i+1):
                if y+z < k:
                    output[y+z] = op(output[y], output[y+z], j != 0)
    return output

def PreOpL2(op, items):
    """
    Uses algorithm from SecureSCM WP9 deliverable.

    op must be a binary function that outputs a new register
    """
    k = len(items)
    half = k / 2
    output = list(items)
    if k == 0:
        return []
    u = [op(items[2 * i], items[2 * i + 1]) for i in range(half)]
    v = PreOpL2(op, u)
    for i in range(half):
        output[2 * i + 1] = v[i]
    for i in range(1,  (k + 1) / 2):
        output[2 * i] = op(v[i - 1], items[2 * i])
    return output

def PreOpN(op, items):
    """ Naive PreOp algorithm """
    k = len(items)
    output = [None]*k
    output[0] = items[0]
    for i in range(1, k):
        output[i] = op(output[i-1], items[i])
    return output

def PreOR(a, kappa=None, raw=False):
    if comparison.const_rounds:
        return PreORC(a, kappa, raw=raw)
    else:
        return PreOpL(or_op, a)

def KOpL(op, a):
    k = len(a)
    if k == 1:
        return a[0]
    else:
        t1 = KOpL(op, a[:k/2])
        t2 = KOpL(op, a[k/2:])
        return op(t1, t2)

def KORL(a, kappa):
    """ log rounds k-ary OR """
    k = len(a)
    if k == 1:
        return a[0]
    else:
        t1 = KORL(a[:k/2], kappa)
        t2 = KORL(a[k/2:], kappa)
        return t1 + t2 - t1*t2

def KORC(a, kappa):
    return PreORC(a, kappa, 1)[0]

def KOR(a, kappa):
    if comparison.const_rounds:
        return KORC(a, kappa)
    else:
        return KORL(a, None)

def KMul(a):
    if comparison.const_rounds:
        return comparison.KMulC(a)
    else:
        return KOpL(mul_op, a)


def Inv(a):
    """ Invert a non-zero value """
    t = [types.sint() for i in range(3)]
    c = [types.cint() for i in range(2)]
    one = types.cint()
    ldi(one, 1)
    inverse(t[0], t[1])
    s = t[0]*a
    asm_open(c[0], s)
    # avoid division by zero for benchmarking
    divc(c[1], one, c[0])
    #divc(c[1], c[0], one)
    return c[1]*t[0]

def BitAdd(a, b, bits_to_compute=None):
    """ Add the bits a[k-1], ..., a[0] and b[k-1], ..., b[0], return k+1
        bits s[0], ... , s[k] """
    k = len(a)
    if not bits_to_compute:
        bits_to_compute = range(k)
    d = [None] * k
    for i in range(1,k):
        #assert(a[i].value == 0 or a[i].value == 1)
        #assert(b[i].value == 0 or b[i].value == 1)
        t = a[i]*b[i]
        d[i] = (a[i] + b[i] - 2*t, t)
        #assert(d[i][0].value == 0 or d[i][0].value == 1)
    d[0] = (None, a[0]*b[0])
    pg = PreOpL(carry, d)
    c = [pair[1] for pair in pg]
    
    # (for testing)
    def print_state():
        print 'a: ',
        for i in range(k):
            print '%d ' % a[i].value,
        print '\nb: ',
        for i in range(k):
            print '%d ' % b[i].value,
        print '\nd: ',
        for i in range(k):
            print '%d ' % d[i][0].value,
        print '\n   ',
        for i in range(k):
            print '%d ' % d[i][1].value,
        print '\n\npg:',
        for i in range(k):
            print '%d ' % pg[i][0].value,
        print '\n    ',
        for i in range(k):
            print '%d ' % pg[i][1].value,
        print ''
    
    for bit in c:
        pass#assert(bit.value == 0 or bit.value == 1)
    s = [None] * (k+1)
    if 0 in bits_to_compute:
        s[0] = a[0] + b[0] - 2*c[0]
        bits_to_compute.remove(0)
    #assert(c[0].value == a[0].value*b[0].value)
    #assert(s[0].value == 0 or s[0].value == 1)
    for i in bits_to_compute:
        s[i] = a[i] + b[i] + c[i-1] - 2*c[i]
        try:
            pass#assert(s[i].value == 0 or s[i].value == 1)
        except AssertionError:
            print '#assertion failed in BitAdd for s[%d]' % i
            print_state()
    s[k] = c[k-1]
    #print_state()
    return s

def BitDec(a, k, m, kappa, bits_to_compute=None):
    if program.Program.prog.options.ring:
        return BitDecRing(a, k, m)
    else:
        return BitDecField(a, k, m, kappa, bits_to_compute)

def BitDecRing(a, k, m):
    n_shift = int(program.Program.prog.options.ring) - m
    r_bits = [types.sint.get_random_bit() for i in range(m)]
    r = types.sint.bit_compose(r_bits)
    shifted = ((a - r) << n_shift).reveal()
    masked = shifted >> n_shift
    return types.intbitint.bit_adder(r_bits, masked.bit_decompose(m))

def BitDecField(a, k, m, kappa, bits_to_compute=None):
    r_dprime = types.sint()
    r_prime = types.sint()
    c = types.cint()
    r = [types.sint() for i in range(m)]
    comparison.PRandM(r_dprime, r_prime, r, k, m, kappa)
    #assert(r_prime.value == sum(r[i].value*2**i for i in range(m)) % comparison.program.P)
    pow2 = two_power(k + kappa)
    asm_open(c, pow2 + two_power(k) + a - two_power(m)*r_dprime - r_prime)
    #rval = 2**m*r_dprime.value + r_prime.value
    #assert(rval % 2**m == r_prime.value)
    #assert(rval == (2**m*r_dprime.value + sum(r[i].value*2**i for i in range(m)) % comparison.program.P ))
    try:
        pass#assert(c.value == (2**(k + kappa) + 2**k + (a.value%2**k) - rval) % comparison.program.P)
    except AssertionError:
        print 'BitDec assertion failed'
        print 'a =', a.value
        print 'a mod 2^%d =' % k, (a.value % 2**k)
    return types.intbitint.bit_adder(list(bits(c,m)), r)


def Pow2(a, l, kappa):
    m = int(ceil(log(l, 2)))
    t = BitDec(a, m, m, kappa)
    x = [types.sint() for i in range(m)]
    pow2k = [types.cint() for i in range(m)]
    for i in range(m):
        pow2k[i] = two_power(2**i)
        t[i] = t[i]*pow2k[i] + 1 - t[i]
    return KMul(t)

def B2U(a, l, kappa):
    pow2a = Pow2(a, l, kappa)
    return B2U_from_Pow2(pow2a, l, kappa), pow2a

def B2U_from_Pow2(pow2a, l, kappa):
    #assert(pow2a.value == 2**a.value)
    r = [types.sint() for i in range(l)]
    t = types.sint()
    c = types.cint()
    for i in range(l):
        bit(r[i])
    comparison.PRandInt(t, kappa)
    asm_open(c, pow2a + two_power(l) * t + sum(two_power(i)*r[i] for i in range(l)))
    comparison.program.curr_tape.require_bit_length(l + kappa)
    c = list(bits(c, l))
    x = [c[i] + r[i] - 2*c[i]*r[i] for i in range(l)]
    #print ' '.join(str(b.value) for b in x)
    y = PreOR(x, kappa)
    #print ' '.join(str(b.value) for b in y)
    return [1 - y[i] for i in range(l)]

def Trunc(a, l, m, kappa, compute_modulo=False):
    """ Oblivious truncation by secret m """
    if l == 1:
        if compute_modulo:
            return a * m, 1 + m
        else:
            return a * (1 - m)
    r = [types.sint() for i in range(l)]
    r_dprime = types.sint(0)
    r_prime = types.sint(0)
    rk = types.sint()
    c = types.cint()
    ci = [types.cint() for i in range(l)]
    d = types.sint()
    x, pow2m = B2U(m, l, kappa)
    #assert(pow2m.value == 2**m.value)
    #assert(sum(b.value for b in x) == m.value)
    if program.Program.prog.options.ring and not compute_modulo:
        return TruncInRing(a, l, pow2m)
    for i in range(l):
        bit(r[i])
        t1 = two_power(i) * r[i]
        t2 = t1*x[i]
        r_prime += t2
        r_dprime += t1 - t2
    #assert(r_prime.value == (sum(2**i*x[i].value*r[i].value for i in range(l)) % comparison.program.P))
    comparison.PRandInt(rk, kappa)
    r_dprime += two_power(l) * rk
    #assert(r_dprime.value == (2**l * rk.value + sum(2**i*(1 - x[i].value)*r[i].value for i in range(l)) % comparison.program.P))
    asm_open(c, a + r_dprime + r_prime)
    for i in range(1,l):
        ci[i] = c % two_power(i)
        #assert(ci[i].value == c.value % 2**i)
    c_dprime = sum(ci[i]*(x[i-1] - x[i]) for i in range(1,l))
    #assert(c_dprime.value == (sum(ci[i].value*(x[i-1].value - x[i].value) for i in range(1,l)) % comparison.program.P))
    lts(d, c_dprime, r_prime, l, kappa)
    if compute_modulo:
        b = c_dprime - r_prime + pow2m * d
        return b, pow2m
    else:
        to_shift = a - c_dprime + r_prime
        if program.Program.prog.options.ring:
            shifted = TruncInRing(to_shift, l, pow2m)
        else:
            pow2inv = Inv(pow2m)
            #assert(pow2inv.value * pow2m.value % comparison.program.P == 1)
            shifted = to_shift * pow2inv
        b = shifted - d
    return b

def TruncInRing(to_shift, l, pow2m):
    n_shift = int(program.Program.prog.options.ring) - l
    bits = BitDecRing(to_shift, l, l)
    rev = types.sint.bit_compose(reversed(bits))
    rev <<= n_shift
    rev *= pow2m
    r_bits = [types.sint.get_random_bit() for i in range(l)]
    r = types.sint.bit_compose(r_bits)
    shifted = (rev - (r << n_shift)).reveal()
    masked = shifted >> n_shift
    bits = types.intbitint.bit_adder(r_bits, masked.bit_decompose(l))
    return types.sint.bit_compose(reversed(bits))

def TruncRoundNearestAdjustOverflow(a, length, target_length, kappa):
    t = comparison.TruncRoundNearest(a, length, length - target_length, kappa)
    overflow = t.greater_equal(two_power(target_length), target_length + 1, kappa)
    if program.Program.prog.options.ring:
        s = (1 - overflow) * t + \
            comparison.TruncLeakyInRing(overflow * t, length, 1, False)
    else:
        s = (1 - overflow) * t + overflow * t / 2
    return s, overflow

def Int2FL(a, gamma, l, kappa):
    lam = gamma - 1
    s = types.sint()
    comparison.LTZ(s, a, gamma, kappa)
    z = EQZ(a, gamma, kappa)
    a = (1 - 2 * s) * a
    a_bits = BitDec(a, lam, lam, kappa)
    a_bits.reverse()
    b = PreOR(a_bits, kappa)
    t = a * (1 + sum(2**i * (1 - b_i) for i,b_i in enumerate(b)))
    p = - (lam - sum(b))
    if gamma - 1 > l:
        if types.sfloat.round_nearest:
            v, overflow = TruncRoundNearestAdjustOverflow(t, gamma - 1, l, kappa)
            p = p + overflow
        else:
            v = types.sint()
            comparison.Trunc(v, t, gamma - 1, gamma - l - 1, kappa, False)
    else:
        v = 2**(l-gamma+1) * t
    p = (p + gamma - 1 - l) * (1 -z)
    return v, p, z, s

def FLRound(x, mode):
    """ Rounding with floating point output.
    *mode*: 0 -> floor, 1 -> ceil, -1 > trunc """
    v1, p1, z1, s1, l, k = x.v, x.p, x.z, x.s, x.vlen, x.plen
    a = types.sint()
    comparison.LTZ(a, p1, k, x.kappa)
    b = p1.less_than(-l + 1, k, x.kappa)
    v2, inv_2pow_p1 = Trunc(v1, l, -a * (1 - b) * x.p, x.kappa, True)
    c = EQZ(v2, l, x.kappa)
    if mode == -1:
        away_from_zero = 0
        mode = x.s
    else:
        away_from_zero = mode + s1 - 2 * mode * s1
    v = v1 - v2 + (1 - c) * inv_2pow_p1 * away_from_zero
    d = v.equal(two_power(l), l + 1, x.kappa)
    v = d * two_power(l-1) + (1 - d) * v
    v = a * ((1 - b) * v + b * away_from_zero * two_power(l-1)) + (1 - a) * v1
    s = (1 - b * mode) * s1
    z = or_op(EQZ(v, l, x.kappa), z1)
    v = v * (1 - z)
    p = ((p1 + d * a) * (1 - b) + b * away_from_zero * (1 - l)) * (1 - z)
    return v, p, z, s

def TruncPr(a, k, m, kappa=None, signed=True):
    """ Probabilistic truncation [a/2^m + u]
        where Pr[u = 1] = (a % 2^m) / 2^m
    """
    if isinstance(a, types.cint):
        return shift_two(a, m)
    if program.Program.prog.options.ring:
        return TruncPrRing(a, k, m, signed=signed)
    else:
        return TruncPrField(a, k, m, kappa)

def TruncPrRing(a, k, m, signed=True):
    if m == 0:
        return a
    n_ring = int(program.Program.prog.options.ring)
    assert n_ring >= k, '%d too large' % k
    if k == n_ring:
        for i in range(m):
            a += types.sint.get_random_bit() << i
        return comparison.TruncLeakyInRing(a, k, m, signed=signed)
    else:
        from types import sint
        # extra bit to mask overflow
        r_bits = [sint.get_random_bit() for i in range(k + 1)]
        n_shift = n_ring - len(r_bits)
        tmp = a + sint.bit_compose(r_bits)
        masked = (tmp << n_shift).reveal()
        shifted = (masked << 1 >> (n_shift + m + 1))
        overflow = r_bits[-1].bit_xor(masked >> (n_ring - 1))
        res = shifted - sint.bit_compose(r_bits[m:k]) + (overflow << (k - m))
        return res

def TruncPrField(a, k, m, kappa=None):
    if kappa is None:
       kappa = 40 

    b = two_power(k-1) + a
    r_prime, r_dprime = types.sint(), types.sint()
    comparison.PRandM(r_dprime, r_prime, [types.sint() for i in range(m)],
                      k, m, kappa)
    two_to_m = two_power(m)
    r = two_to_m * r_dprime + r_prime
    c = (b + r).reveal()
    c_prime = c % two_to_m
    a_prime = c_prime - r_prime
    d = (a - a_prime) / two_to_m
    return d

def SDiv(a, b, l, kappa, round_nearest=False):
    theta = int(ceil(log(l / 3.5) / log(2)))
    alpha = two_power(2*l)
    w = types.cint(int(2.9142 * two_power(l))) - 2 * b
    x = alpha - b * w
    y = a * w
    y = y.round(2 * l + 1, l, kappa, round_nearest)
    x2 = types.sint()
    comparison.Mod2m(x2, x, 2 * l + 1, l, kappa, False)
    x1 = comparison.TruncZeroes(x - x2, 2 * l + 1, l, True)
    for i in range(theta-1):
        y = y * (x1 + two_power(l)) + (y * x2).round(2 * l, l, kappa, round_nearest)
        y = y.round(2 * l + 1, l + 1, kappa, round_nearest)
        x = x1 * x2 + (x2**2).round(2 * l + 1, l + 1, kappa, round_nearest)
        x = x1 * x1 + x.round(2 * l + 1, l - 1, kappa, round_nearest)
        x2 = types.sint()
        comparison.Mod2m(x2, x, 2 * l, l, kappa, False)
        x1 = comparison.TruncZeroes(x - x2, 2 * l + 1, l, True)
    y = y * (x1 + two_power(l)) + (y * x2).round(2 * l, l, kappa, round_nearest)
    y = y.round(2 * l + 1, l - 1, kappa, round_nearest)
    return y

def SDiv_mono(a, b, l, kappa):
    theta = int(ceil(log(l / 3.5) / log(2)))
    alpha = two_power(2*l)
    w = types.cint(int(2.9142 * two_power(l))) - 2 * b
    x = alpha - b * w
    y = a * w
    y = TruncPr(y, 2 * l + 1, l + 1, kappa)
    for i in range(theta-1):
        y = y * (alpha + x)
        # keep y with l bits
        y = TruncPr(y, 3 * l, 2 * l, kappa)
        x = x**2
        # keep x with 2l bits
        x = TruncPr(x, 4 * l, 2 * l, kappa)
    y = y * (alpha + x)
    y = TruncPr(y, 3 * l, 2 * l, kappa)
    return y


def FPDiv(a, b, k, f, kappa):
    theta = int(ceil(log(k/3.5)))
    alpha = types.cint(1 * two_power(2*f))

    w = AppRcr(b, k, f, kappa)
    x = alpha - b * w
    y = a * w
    y = TruncPr(y, 2*k, f, kappa)

    for i in range(theta):
        y = y * (alpha + x)
        x = x * x
        y = TruncPr(y, 2*k, 2*f, kappa)
        x = TruncPr(x, 2*k, 2*f, kappa)

    y = y * (alpha + x)
    y = TruncPr(y, 2*k, 2*f, kappa)
    return y

def AppRcr(b, k, f, kappa):
    """
        Approximate reciprocal of [b]:
        Given [b], compute [1/b]
    """
    alpha = types.cint(int(2.9142 * (2**k)))
    c, v = Norm(b, k, f, kappa)
    d = alpha - 2 * c
    w = d * v
    w = TruncPr(w, 2 * k, 2 * (k - f))
    return w

def Norm(b, k, f, kappa):
    """
        Computes secret integer values [c] and [v_prime] st.
        2^{k-1} <= c < 2^k and c = b*v_prime
    """
    temp = types.sint()
    comparison.LTZ(temp, b, k, kappa)
    sign = 1 - 2 * temp # 1 - 2 * [b < 0]

    x = sign * b
    #x = |b|
    bits = x.bit_decompose(k)
    y = PreOR(bits)

    z = [0] * k
    for i in range(k - 1):
        z[i] = y[i] - y[i + 1]

    z[k - 1] = y[k - 1]
    # z[i] = 0 for all i except when bits[i + 1] = first one

    #now reverse bits of z[i]
    v = types.sint()
    for i in range(k):
        v += two_power(k - i - 1) * z[i]
    c = x * v
    v_prime = sign * v
    return c, v_prime

