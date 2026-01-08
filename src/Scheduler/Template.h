#ifndef _HARMONIC_SCHEDULER_TEMPLATE_h
#define _HARMONIC_SCHEDULER_TEMPLATE_h

#include "Abstract.h"
#include "NoProfiling.h"
#include "BaseProfiling.h"
#include "FullProfiling.h"

namespace Harmonic
{
	namespace Selector
	{
		template<task_id_t MaxTaskCount, bool IdleSleepEnabled, ProfileLevelEnum  Level>
		struct TemplateSchedulerSelector;

		template<task_id_t MaxTaskCount, bool IdleSleepEnabled>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfileLevelEnum::None>
		{
			using Type = SchedulerNoProfiling<MaxTaskCount, IdleSleepEnabled>;
		};

		template<task_id_t MaxTaskCount, bool IdleSleepEnabled>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfileLevelEnum::Base>
		{
			using Type = SchedulerBaseProfiling<MaxTaskCount, IdleSleepEnabled>;
		};

		template<task_id_t MaxTaskCount, bool IdleSleepEnabled>
		struct TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, ProfileLevelEnum::Full>
		{
			using Type = SchedulerFullProfiling<MaxTaskCount, IdleSleepEnabled>;
		};
	}

	template<task_id_t MaxTaskCount, bool IdleSleepEnabled = false, ProfileLevelEnum  Level = ProfileLevelEnum::None>
	using TemplateScheduler = typename Selector::TemplateSchedulerSelector<MaxTaskCount, IdleSleepEnabled, Level>::Type;
}
#endif