#pragma once

#include "Env_Strat_FirstFrame_Base.hpp"

namespace Renderer
{
	class Env_Strat_FirstFrame_Init final : public Env_Strat_FirstFrame_Base
	{
	public:
		Env_Strat_FirstFrame_Init() = default;
		~Env_Strat_FirstFrame_Init() = default;

		Env_Strat_FirstFrame_Init(const Env_Strat_FirstFrame_Init&) = delete;
		Env_Strat_FirstFrame_Init& operator=(const Env_Strat_FirstFrame_Init&) = delete;

		Env_Strat_FirstFrame_Base* Execute(Environment* environment) override;
	};

	static Env_Strat_FirstFrame_Init ENV_STRAT_FIRSTFRAME_INIT;
}
