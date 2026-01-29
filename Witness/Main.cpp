#include "Witness.h"

auto main(int p_Arg_c, char** p_Args_v) noexcept -> int
{
    Witness* wit = new Witness();
    wit->LoadMap("Witness/testmap.txt");
    wit->LoadModel("Witness/Model.bin");
    wit->Learn();
    wit->PrintModel();
    delete wit;
    return 0;
}
