#pragma once

namespace Renderer
{
	class Environment;
}

namespace Renderer
{
	class Env_Strat_FirstFrame_Base
	{
		public:
			Env_Strat_FirstFrame_Base() = default;
			~Env_Strat_FirstFrame_Base() = default;

			Env_Strat_FirstFrame_Base(const Env_Strat_FirstFrame_Base&) = delete;
			Env_Strat_FirstFrame_Base& operator=(const Env_Strat_FirstFrame_Base&) = delete;

			virtual Env_Strat_FirstFrame_Base* Execute(Environment* environment) = 0;
	};

	using FirstFrameStrat = Env_Strat_FirstFrame_Base;
}
