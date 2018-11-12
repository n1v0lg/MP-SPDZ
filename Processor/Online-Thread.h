#ifndef _Online_Thread
#define _Online_Thread

#include "Networking/Player.h"
#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Integer.h"
#include "Processor/Data_Files.h"

#include <vector>
using namespace std;

template<class sint> class Machine;

template<class sint>
class thread_info
{
  public: 

  int thread_num;
  int covert;
  Names*  Nms;
  gf2n *alpha2i;
//  gfp  *alphapi;
//  sint;
  typename sint::value_type *alphapi;
  int prognum;
  bool finished;
  bool ready;

  // rownums for triples, bits, squares, and inverses etc
  DataPositions pos;
  // Integer arg (optional)
  int arg;

  Machine<sint>* machine;

  static void* Main_Func(void *ptr);

  static void purge_preprocessing(Names& N, string prep_dir);
};

#endif

