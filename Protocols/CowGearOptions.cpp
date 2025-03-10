/*
 * CowGearOptions.cpp
 *
 */

#include "CowGearOptions.h"
#include "Tools/benchmarking.h"

#include <math.h>
#include <string>
using namespace std;

CowGearOptions CowGearOptions::singleton;

CowGearOptions::CowGearOptions()
{
    covert_security = 20;
    lowgear_from_covert();
}

void CowGearOptions::lowgear_from_covert()
{
    lowgear_security = ceil(log2(covert_security));
}

CowGearOptions::CowGearOptions(ez::ezOptionParser& opt, int argc,
        const char** argv) : CowGearOptions()
{
    opt.add(
            "", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            ("Covert security parameter c. "
                    "Cheating will be detected with probability 1/c (default: "
                    + to_string(covert_security) + ")").c_str(), // Help description.
            "-c", // Flag token.
            "--covert-security" // Flag token.
    );
    opt.add(
            "", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "LowGear security parameter (default: ceil(log2(c))", // Help description.
            "-l", // Flag token.
            "--lowgear-security" // Flag token.
    );
    opt.parse(argc, argv);
    if (opt.isSet("-c"))
        opt.get("-c")->getInt(covert_security);
    if (opt.isSet("-l"))
    {
        opt.get("-l")->getInt(lowgear_security);
        if (lowgear_security <= 0)
        {
            throw exception();
            cerr << "Invalid LowGear Security parameter: " << lowgear_security << endl;
            exit(1);
        }
        if (covert_security > (1 << lowgear_security))
            insecure("LowGear security less than key generation security");
    }
    else
        lowgear_from_covert();
    opt.resetArgs();
}
