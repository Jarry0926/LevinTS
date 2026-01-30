#include "Witness.h"

auto main(int p_Arg_c, char** p_Args_v) noexcept -> int
{
    Witness* wit = new Witness();
    wit->LoadMap("Witness/testmap.txt");
    wit->LoadModel("Witness/Model.bin");
    if (wit->Learn()) printf("Solved.\n");
    else printf("Didn't solve.\n");
    wit->PrintModel();
    delete wit;
    return 0;
}
