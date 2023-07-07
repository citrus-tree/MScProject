#include "Env_Strat_FirstFrame_Init.h"

/* renderer */
#include "Environment.hpp"
#include "Env_Strat_FirstFrame_Null.hpp"

Renderer::Env_Strat_FirstFrame_Base* Renderer::Env_Strat_FirstFrame_Init::Execute(Environment* environment)
{
	environment->CmdPrimeIntermediates();

	return &ENV_STRAT_FIRSTFRAME_NULL;
}
