#ifndef _Player
#define _Player

/* Class to create a player, for KeyGen, Offline and Online phases.
 *
 * Basically handles connection to the server to obtain the names
 * of the other players. Plus sending and receiving of data
 *
 */

#include <vector>
#include <set>
#include <iostream>
#include <fstream>
using namespace std;

#include "Tools/octetStream.h"
#include "Tools/FlexBuffer.h"
#include "Networking/sockets.h"
#include "Networking/ServerSocket.h"
#include "Tools/sha1.h"
#include "Tools/int.h"
#include "Networking/Receiver.h"
#include "Networking/Sender.h"

template<class T> class MultiPlayer;

/* Class to get the names off the server */
class Names
{
  vector<string> names;
  vector<int> ports;
  int nplayers;
  int portnum_base;
  int player_no;

  int default_port(int playerno) { return portnum_base + playerno; }
  void setup_ports();

  void setup_names(const char *servername, int my_port);

  void setup_server();

  public:

  static const int DEFAULT_PORT = -1;

  mutable ServerSocket* server;

  void init(int player,int pnb,int my_port,const char* servername);
  Names(int player,int pnb,int my_port,const char* servername)
    { init(player,pnb,my_port,servername); }
  // Set up names when we KNOW who we are going to be using before hand
  void init(int player,int pnb,vector<octet*> Nms);
  Names(int player,int pnb,vector<octet*> Nms)
    { init(player,pnb,Nms); }
  void init(int player,int pnb,vector<string> Nms);
  Names(int player,int pnb,vector<string> Nms)
    { init(player,pnb,Nms); }
  // nplayers = 0 for taking it from hostsfile
  void init(int player, int pnb, const string& hostsfile, int players = 0);
  Names(int player, int pnb, const string& hostsfile)
    { init(player, pnb, hostsfile); }

  Names() : nplayers(-1), portnum_base(-1), player_no(-1), server(0) { ; }
  Names(const Names& other);
  ~Names();

  int num_players() const { return nplayers; }
  int my_num() const { return player_no; }
  const string get_name(int i) const { return names[i]; }
  int get_portnum_base() const { return portnum_base; }

  friend class PlayerBase;
  friend class Player;
  template<class T> friend class MultiPlayer;
  friend class RealTwoPartyPlayer;
};


struct CommStats
{
  size_t data, rounds;
  Timer timer;
  CommStats() : data(0), rounds(0) {}
  Timer& add(const octetStream& os) { data += os.get_length(); rounds++; return timer; }
  CommStats& operator+=(const CommStats& other);
};

class NamedCommStats : public map<string, CommStats>
{
public:
  NamedCommStats& operator+=(const NamedCommStats& other);
  size_t total_data();
#ifdef VERBOSE_COMM
  CommStats& operator[](const string& name)
  {
    auto& res = map<string, CommStats>::operator[](name);
    cout << name << " after " << res.data << endl;
    return res;
  }
#endif
};

class PlayerBase
{
protected:
  int player_no;

public:
  mutable size_t sent;
  mutable Timer timer;
  mutable NamedCommStats comm_stats;

  PlayerBase(int player_no) : player_no(player_no), sent(0) {}
  virtual ~PlayerBase();

  int my_real_num() const { return player_no; }
  virtual int num_players() const = 0;

  virtual void pass_around(octetStream& o, int offset = 1) const = 0;
};

class Player : public PlayerBase
{
protected:
  int nplayers;

  mutable blk_SHA_CTX ctx;

public:
  const Names& N;

  Player(const Names& Nms);
  virtual ~Player();

  int num_players() const { return nplayers; }
  int my_num() const { return player_no; }

  int get_offset(int other_player) const { return positive_modulo(other_player - my_num(), num_players()); }
  int get_player(int offset) const { return positive_modulo(offset + my_num(), num_players()); }

  virtual bool is_encrypted() { return false; }

  virtual void send_long(int i, long a) const = 0;
  virtual long receive_long(int i) const = 0;

  virtual void send_all(const octetStream& o,bool donthash=false) const = 0;
  void send_to(int player,const octetStream& o,bool donthash=false) const;
  virtual void send_to_no_stats(int player,const octetStream& o) const = 0;
  void receive_player(int i,octetStream& o,bool donthash=false) const;
  virtual void receive_player_no_stats(int i,octetStream& o) const = 0;
  virtual void receive_player(int i,FlexBuffer& buffer) const;

  // Communication relative to my number
  void send_relative(const vector<octetStream>& o) const;
  void send_relative(int offset, const octetStream& o) const;
  void receive_relative(vector<octetStream>& o) const;
  void receive_relative(int offset, octetStream& o) const;

  // exchange data with minimal memory usage
  void exchange(int other, const octetStream& to_send, octetStream& ot_receive) const;
  virtual void exchange_no_stats(int other, const octetStream& to_send, octetStream& ot_receive) const = 0;
  void exchange(int other, octetStream& o) const;
  void exchange_relative(int offset, octetStream& o) const;
  void pass_around(octetStream& o, int offset = 1) const { pass_around(o, o, offset); }
  void pass_around(octetStream& to_send, octetStream& to_receive, int offset) const;
  virtual void pass_around_no_stats(octetStream& to_send, octetStream& to_receive, int offset) const = 0;

  /* Broadcast and Receive data to/from all players
   *  - Assumes o[player_no] contains the thing broadcast by me
   */
  virtual void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const = 0;

  /* Run Protocol To Verify Broadcast Is Correct
   *     - Resets the blk_SHA_CTX at the same time
   */
  virtual void Check_Broadcast() const;

  // dummy functions for compatibility
  virtual void request_receive(int i, octetStream& o) const { (void)i; (void)o; }
  virtual void wait_receive(int i, octetStream& o, bool donthash=false) const { receive_player(i, o, donthash); }
};

template<class T>
class MultiPlayer : public Player
{
protected:
  vector<T> sockets;
  T send_to_self_socket;

  void setup_sockets(const vector<string>& names,const vector<int>& ports,int id_base,ServerSocket& server);

  map<T,int> socket_players;

  T socket_to_send(int player) const { return player == player_no ? send_to_self_socket : sockets[player]; }

  friend class CryptoPlayer;

public:
  // The offset is used for the multi-threaded call, to ensure different
  // portnum bases in each thread
  MultiPlayer(const Names& Nms,int id_base=0);

  virtual ~MultiPlayer();

  T socket(int i) const { return sockets[i]; }

  // Send/Receive data to/from player i 
  // 8-bit ints only (mainly for testing)
  void send_int(int i,int a)  const    { send(sockets[i],a);    }
  void receive_int(int i,int& a) const { receive(sockets[i],a); }

  void send_long(int i, long a) const;
  long receive_long(int i) const;

  // Send an octetStream to all other players 
  //   -- And corresponding receive
  virtual void send_all(const octetStream& o,bool donthash=false) const;
  void send_to_no_stats(int player,const octetStream& o) const;
  virtual void receive_player_no_stats(int i,octetStream& o) const;

  // exchange data with minimal memory usage
  void exchange_no_stats(int other, const octetStream& to_send, octetStream& ot_receive) const;

  // send to next and receive from previous player
  virtual void pass_around_no_stats(octetStream& to_send, octetStream& to_receive, int offset) const;

  // Receive one from player i

  /* Broadcast and Receive data to/from all players 
   *  - Assumes o[player_no] contains the thing broadcast by me
   */
  void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const;

  // wait for available inputs
  void wait_for_available(vector<int>& players, vector<int>& result) const;
};

typedef MultiPlayer<int> PlainPlayer;


class ThreadPlayer : public MultiPlayer<int>
{
public:
  mutable vector<Receiver*> receivers;
  mutable vector<Sender<int>*>   senders;

  ThreadPlayer(const Names& Nms,int id_base=0);
  virtual ~ThreadPlayer();

  void request_receive(int i, octetStream& o) const;
  void wait_receive(int i, octetStream& o, bool donthash=false) const;
  void receive_player_no_stats(int i,octetStream& o) const;

  void send_all(const octetStream& o,bool donthash=false) const;
};


class TwoPartyPlayer : public PlayerBase
{
public:
  TwoPartyPlayer(int my_num) : PlayerBase(my_num) {}
  virtual ~TwoPartyPlayer() {}

  virtual int my_num() const = 0;
  virtual int other_player_num() const = 0;

  virtual void send(octetStream& o) = 0;
  virtual void receive(octetStream& o) = 0;
  virtual void send_receive_player(vector<octetStream>& o) = 0;
};

class RealTwoPartyPlayer : public TwoPartyPlayer
{
private:
  // setup sockets for comm. with only one other player
  void setup_sockets(int other_player, const Names &nms, int portNum, int id);

  int socket;
  bool is_server;
  int other_player;

public:
  RealTwoPartyPlayer(const Names& Nms, int other_player, int pn_offset=0);
  ~RealTwoPartyPlayer();

  void send(octetStream& o);
  void receive(octetStream& o);

  int other_player_num() const;
  int my_num() const { return is_server; }
  int num_players() const { return 2; }

  /* Send and receive to/from the other player
   *  - o[0] contains my data, received data put in o[1]
   */
  void send_receive_player(vector<octetStream>& o);

  void exchange(octetStream& o) const;
  void exchange(int other, octetStream& o) const { (void)other; exchange(o); }
  void pass_around(octetStream& o, int offset = 1) const { (void)offset; exchange(o); }
};

// for different threads, separate statistics
class VirtualTwoPartyPlayer : public TwoPartyPlayer
{
  Player& P;
  int other_player;

public:
  VirtualTwoPartyPlayer(Player& P, int other_player);

  // emulate RealTwoPartyPlayer
  int my_num() const { return P.my_num() > other_player; }
  int other_player_num() const { return other_player; }
  int num_players() const { return 2; }

  void send(octetStream& o);
  void receive(octetStream& o);
  void send_receive_player(vector<octetStream>& o);

  void pass_around(octetStream& o, int _ = 1) const { (void)_, (void) o; throw not_implemented(); }
};

// for the same thread
class OffsetPlayer : public TwoPartyPlayer
{
private:
  Player& P;
  int offset;

public:
  OffsetPlayer(Player& P, int offset) : TwoPartyPlayer(P.my_num()), P(P), offset(offset) {}

  // emulate RealTwoPartyPlayer
  int my_num() const { return P.my_num() > P.get_player(offset); }
  int other_player_num() const { return P.get_player(offset); }
  int num_players() const { return 2; }
  int get_offset() const { return offset; }

  void send(octetStream& o) { P.send_to(P.get_player(offset), o, true); }
  void reverse_send(octetStream& o) { P.send_to(P.get_player(-offset), o, true); }
  void receive(octetStream& o) { P.receive_player(P.get_player(offset), o, true); }
  void reverse_receive(octetStream& o) { P.receive_player(P.get_player(-offset), o, true); }
  void send_receive_player(vector<octetStream>& o);

  void reverse_exchange(octetStream& o) const { P.pass_around(o, P.num_players() - offset); }
  void exchange(int other, octetStream& o) const { (void)other; P.pass_around(o, offset); }
  void pass_around(octetStream& o, int _ = 1) const { (void)_; P.pass_around(o, offset); }
  void Broadcast_Receive(vector<octetStream>& o,bool donthash=false) const;
};

#endif
