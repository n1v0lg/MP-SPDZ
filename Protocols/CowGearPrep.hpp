/*
 * CowGearPrep.cpp
 *
 */

#include "CowGearPrep.h"
#include "FHEOffline/PairwiseMachine.h"
#include "Tools/Bundle.h"

template<class T>
PairwiseMachine* CowGearPrep<T>::pairwise_machine = 0;
template<class T>
Lock CowGearPrep<T>::lock;

template<class T>
CowGearPrep<T>::~CowGearPrep()
{
    if (pairwise_generator)
        delete pairwise_generator;
}

template<class T>
void CowGearPrep<T>::teardown()
{
    if (pairwise_machine)
        delete pairwise_machine;
}

template<class T>
void CowGearPrep<T>::setup(Player& P, mac_key_type alphai)
{
    Timer timer;
    timer.start();
    assert(pairwise_machine == 0);
    pairwise_machine = new PairwiseMachine(P);
    auto& machine = *pairwise_machine;
    auto& setup = machine.setup<FD>();
    auto& options = CowGearOptions::singleton;
#ifdef VERBOSE
    cerr << "Covert security parameter for key and MAC generation: "
            << options.covert_security << endl;
    cerr << "LowGear security parameter: " << options.lowgear_security << endl;
#endif
    setup.secure_init(P, machine, T::clear::length(), options.lowgear_security);
    setup.covert_key_generation(P, machine, options.covert_security);
    setup.covert_mac_generation(P, machine, options.covert_security);

    // adjust mac key
    mac_key_type diff = alphai - setup.alphai;
    setup.set_alphai(alphai);
    Bundle<octetStream> bundle(P);
    diff.pack(bundle.mine);
    P.Broadcast_Receive(bundle, true);
    for (int i = 0; i < P.num_players(); i++)
    {
        Plaintext_<FD> mess(setup.FieldD);
        mess.assign_constant(bundle[i].get<mac_key_type>(), Polynomial);
        machine.enc_alphas[i] += mess;
    }

    // generate minimal number of items
    machine.nTriplesPerThread = 1;
#ifdef VERBOSE
    cerr << T::type_string() << " setup took " << timer.elapsed() << " seconds" << endl;
#endif
}

template<class T>
PairwiseGenerator<typename T::clear::FD>& CowGearPrep<T>::get_generator()
{
    auto& proc = this->proc;
    assert(proc != 0);
    lock.lock();
    if (pairwise_machine == 0)
    {
        PlainPlayer P(proc->P.N, T::clear::type_char());
        setup(P, proc->MC.get_alphai());
    }
    lock.unlock();
    if (pairwise_generator == 0)
    {
        auto& machine = *pairwise_machine;
        typedef typename T::open_type::FD FD;
        // generate minimal number of items
        this->buffer_size = min(machine.setup<FD>().alpha.num_slots(),
                (unsigned)OnlineOptions::singleton.batch_size);
        pairwise_generator = new PairwiseGenerator<FD>(0, machine, &proc->P);
    }
    return *pairwise_generator;
}

template<class T>
void CowGearPrep<T>::buffer_triples()
{
    auto& generator = get_generator();
    generator.run();
    auto& producer = generator.producer;
    assert(not producer.triples.empty());
    for (auto& triple : producer.triples)
        this->triples.push_back({{triple[0], triple[1], triple[2]}});
#ifdef VERBOSE
    cerr << "generated " << producer.triples.size() << " triples, now got "
            << this->triples.size() << endl;
#endif
}

template<class T>
void CowGearPrep<T>::buffer_inverses()
{
    assert(this->proc != 0);
    BufferPrep<T>::buffer_inverses(this->proc->MC, this->proc->P);
}

template<class T>
void CowGearPrep<T>::buffer_inputs(int player)
{
    auto& generator = get_generator();
    generator.generate_inputs(player);
    assert(not generator.inputs.empty());
    this->inputs.resize(this->proc->P.num_players());
    for (auto& input : generator.inputs)
        this->inputs[player].push_back(input);
#ifdef VERBOSE
    cerr << "generated " << generator.inputs.size() << " inputs, now got "
            << this->inputs[player].size() << endl;
#endif
}

template<>
inline void CowGearPrep<CowGearShare<gfp>>::buffer_bits()
{
    buffer_bits_from_squares(*this);
}

template<>
inline void CowGearPrep<CowGearShare<gf2n_short>>::buffer_bits()
{
    buffer_bits_without_check();
    assert(not this->bits.empty());
    for (auto& bit : this->bits)
        bit.force_to_bit();
}
