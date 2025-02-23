/*
 * Multiplier.cpp
 *
 */

#include <FHEOffline/Multiplier.h>
#include "FHEOffline/PairwiseGenerator.h"
#include "FHEOffline/PairwiseMachine.h"

template <class FD>
Multiplier<FD>::Multiplier(int offset, PairwiseGenerator<FD>& generator) :
    generator(generator), machine(generator.machine),
    P(generator.P, offset),
    num_players(generator.P.num_players()),
    my_num(generator.P.my_num()),
    other_pk(machine.other_pks[(my_num + num_players - offset) % num_players]),
    other_enc_alpha(machine.enc_alphas[(my_num + num_players - offset) % num_players]),
    timers(generator.timers),
    C(machine.pk), mask(machine.pk),
    product_share(machine.setup<FD>().FieldD), rc(machine.pk),
    volatile_capacity(0)
{
    product_share.allocate_slots(machine.setup<FD>().params.p0() << 64);
}

template <class FD>
void Multiplier<FD>::multiply_and_add(Plaintext_<FD>& res,
        const Ciphertext& enc_a, const Plaintext_<FD>& b)
{
    Rq_Element bb(enc_a.get_params(), evaluation, evaluation);
    bb.from_vec(b.get_poly());
    multiply_and_add(res, enc_a, bb);
}

template <class FD>
void Multiplier<FD>::multiply_and_add(Plaintext_<FD>& res,
        const Ciphertext& enc_a, const Rq_Element& b, OT_ROLE role)
{
    octetStream o;

    if (role & SENDER)
    {
        PRNG G;
        G.ReSeed();
        timers["Ciphertext multiplication"].start();
        C.mul(enc_a, b);
        timers["Ciphertext multiplication"].stop();
        timers["Mask randomization"].start();
        product_share.randomize(G);
        bigint B = 6 * machine.setup<FD>().params.get_R();
        B *= machine.setup<FD>().FieldD.get_prime();
        B <<= machine.drown_sec;
        // slack
        B *= NonInteractiveProof::slack(machine.sec,
                machine.setup<FD>().params.phi_m());
        B <<= machine.extra_slack;
        rc.generateUniform(G, 0, B, B);
        timers["Mask randomization"].stop();
        timers["Encryption"].start();
        other_pk.encrypt(mask, product_share, rc);
        timers["Encryption"].stop();
        mask += C;
        mask.pack(o);
        res -= product_share;
    }

    timers["Multiplied ciphertext sending"].start();
    if (role == BOTH)
        P.reverse_exchange(o);
    else if (role == SENDER)
        P.reverse_send(o);
    else if (role == RECEIVER)
        P.receive(o);
    timers["Multiplied ciphertext sending"].stop();

    if (role & RECEIVER)
    {
        timers["Decryption"].start();
        C.unpack(o);
        machine.sk.decrypt_any(product_share, C);
        res += product_share;
        timers["Decryption"].stop();
    }

    memory_usage.update("multiplied ciphertext", C.report_size(CAPACITY));
    memory_usage.update("mask ciphertext", mask.report_size(CAPACITY));
    memory_usage.update("product shares", product_share.report_size(CAPACITY));
    memory_usage.update("masking random coins", rc.report_size(CAPACITY));
}

template <class FD>
void Multiplier<FD>::multiply_alpha_and_add(Plaintext_<FD>& res,
        const Rq_Element& b, OT_ROLE role)
{
    multiply_and_add(res, other_enc_alpha, b, role);
}

template <class FD>
size_t Multiplier<FD>::report_size(ReportType type)
{
    return C.report_size(type) + mask.report_size(type)
            + product_share.report_size(type) + rc.report_size(type);
}

template <class FD>
void Multiplier<FD>::report_size(ReportType type, MemoryUsage& res)
{
    (void)type;
    res += memory_usage;
}


template class Multiplier<FFT_Data>;
template class Multiplier<P2Data>;
