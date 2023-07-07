#pragma once

#include "Env_Strat_FirstFrame_Base.hpp"

namespace Renderer
{
	class Env_Strat_FirstFrame_Null final : public Env_Strat_FirstFrame_Base
	{
	public:
		Env_Strat_FirstFrame_Null() = default;
		~Env_Strat_FirstFrame_Null() = default;

		Env_Strat_FirstFrame_Null(const Env_Strat_FirstFrame_Null&) = delete;
		Env_Strat_FirstFrame_Null& operator=(const Env_Strat_FirstFrame_Null&) = delete;

		inline Env_Strat_FirstFrame_Base* Execute(Environment* environment) override { return this; };
	};

	static Env_Strat_FirstFrame_Null ENV_STRAT_FIRSTFRAME_NULL;
}
