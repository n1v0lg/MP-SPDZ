/*
 * PairwiseGenerator.cpp
 *
 */

#include "FHEOffline/PairwiseGenerator.h"
#include "FHEOffline/PairwiseMachine.h"
#include "FHEOffline/Producer.h"
#include "Protocols/SemiShare.h"

#include "Protocols/MAC_Check.hpp"
#include "Protocols/SemiInput.hpp"
#include "Protocols/ReplicatedInput.hpp"
#include "Processor/Input.hpp"

template <class FD>
PairwiseGenerator<FD>::PairwiseGenerator(int thread_num,
        PairwiseMachine& machine, Player* player) :
    GeneratorBase(thread_num, machine.N, player),
    producer(machine.setup<FD>().FieldD, P.my_num(),
            thread_num, machine.output),
    EC(P, machine.other_pks, machine.setup<FD>().FieldD, timers, machine, *this),
    MC(machine.setup<FD>().alphai),
    C(machine.sec, machine.setup<FD>().params), volatile_memory(0),
    machine(machine)
{
    for (int i = 1; i < P.num_players(); i++)
        multipliers.push_back(new Multiplier<FD>(i, *this));
    const FD& FieldD = machine.setup<FD>().FieldD;
    a.resize(machine.sec, FieldD);
    b.resize(machine.sec, FieldD);
    c.resize(machine.sec, FieldD);
    a.allocate_slots(FieldD.get_prime());
    b.allocate_slots(FieldD.get_prime());
    // extra limb for addition
    c.allocate_slots((bigint)FieldD.get_prime() << 64);
    b_mod_q.resize(machine.sec,
    { machine.setup<FD>().params, evaluation, evaluation });
}

template <class FD>
PairwiseGenerator<FD>::~PairwiseGenerator()
{
    for (auto m : multipliers)
        delete m;
}

template <class FD>
void PairwiseGenerator<FD>::run()
{
    PRNG G;
    G.ReSeed();

    while (total < machine.nTriplesPerThread)
    {
        timers["Randomization"].start();
        a.randomize(G);
        b.randomize(G);
        timers["Randomization"].stop();
        size_t prover_memory = EC.generate_proof(C, a, ciphertexts, cleartexts);
        timers["Plaintext multiplication"].start();
        c.mul(a, b);
        timers["Plaintext multiplication"].stop();
        timers["FFT of b"].start();
        for (int i = 0; i < machine.sec; i++)
            b_mod_q.at(i).from_vec(b.at(i).get_poly());
        timers["FFT of b"].stop();
        timers["Proof exchange"].start();
        size_t verifier_memory = EC.create_more(ciphertexts, cleartexts);
        timers["Proof exchange"].stop();
        volatile_memory = max(prover_memory, verifier_memory);

        Rq_Element values({machine.setup<FD>().params, evaluation, evaluation});
        for (int k = 0; k < machine.sec; k++)
        {
            producer.ai = a[k];
            producer.bi = b[k];
            producer.ci = c[k];

            for (int j = 0; j < 3; j++)
            {
                timers["Plaintext multiplication"].start();
                producer.macs[j].mul(machine.setup<FD>().alpha, producer.values[j]);
                timers["Plaintext multiplication"].stop();

                if (j == 1)
                    values = b_mod_q[k];
                else
                {
                    timers["Plaintext conversion"].start();
                    values.from_vec(producer.values[j].get_poly());
                    timers["Plaintext conversion"].stop();
                }

                for (auto m : multipliers)
                    m->multiply_alpha_and_add(producer.macs[j], values);
            }
            producer.reset();
            total += producer.sacrifice(P, MC);
        }

        timers["Checking"].start();
        MC.Check(P);
        timers["Checking"].stop();
    }

#ifdef FHE_MEMORY
    cerr << "Could save " << 1e-9 * a.report_size(CAPACITY) << " GB" << endl;
#endif
    timers.insert(EC.timers.begin(), EC.timers.end());
    timers.insert(producer.timers.begin(), producer.timers.end());
    timers["Networking"] = P.timer;
}

template <class FD>
void PairwiseGenerator<FD>::generate_inputs(int player)
{
    bool mine = player == P.my_num();
    if (mine)
    {
        SeededPRNG G;
        b[0].randomize(G);
        b_mod_q.at(0).from_vec(b.at(0).get_poly());
        producer.macs[0].mul(machine.setup<FD>().alpha, b[0]);
    }
    else
        producer.macs[0].assign_zero();

    for (auto m : multipliers)
        if (mine or P.get_player(m->get_offset()) == player)
            m->multiply_alpha_and_add(producer.macs[0], b_mod_q[0], mine ? SENDER : RECEIVER);

    inputs.clear();
    Share<T> check_value;
    GlobalPRNG G(P);
    SemiInput<SemiShare<T>> input(0, P);
    input.reset_all(P);
    for (size_t i = 0; i < b[0].num_slots(); i++)
    {
        input.add_mine(b[0].element(i));
    }
    input.exchange();
    for (size_t i = 0; i < b[0].num_slots(); i++)
    {
        Share<T> share(input.finalize(player), producer.macs[0].element(i));
        inputs.push_back({share, b[0].element(i)});
        check_value += G.get<T>() * share;
    }
    inputs.pop_back();
    MC.POpen(check_value, P);
    MC.Check(P);
}

template <class FD>
size_t PairwiseGenerator<FD>::report_size(ReportType type)
{
    size_t res = a.report_size(type) + b.report_size(type) + c.report_size(type);
    for (auto m : multipliers)
        res += m->report_size(type);
    res += producer.report_size(type);
    res += multipliers[0]->report_volatile();
    res += volatile_memory + C.report_size(type);
    res += ciphertexts.get_max_length() + cleartexts.get_max_length();
    res += EC.report_size(type) + EC.volatile_memory;
    res += b_mod_q.report_size(type);
    return res;
}

template <class FD>
size_t PairwiseGenerator<FD>::report_sent()
{
    return P.sent;
}

template <class FD>
void PairwiseGenerator<FD>::report_size(ReportType type, MemoryUsage& res)
{
    multipliers[0]->report_size(type, res);
    res.add("shares",
            a.report_size(type) + b.report_size(type) + c.report_size(type));
    res.add("producer", producer.report_size(type));
    res.add("my ciphertexts", C.report_size(CAPACITY));
    res.add("serialized ciphertexts", ciphertexts.get_max_length());
    res.add("serialized cleartexts", cleartexts.get_max_length());
    res.add("generator volatile", volatile_memory);
    res.add("b mod p", b_mod_q.report_size(type));
    res += EC.memory_usage;
}

template class PairwiseGenerator<FFT_Data>;
template class PairwiseGenerator<P2Data>;
