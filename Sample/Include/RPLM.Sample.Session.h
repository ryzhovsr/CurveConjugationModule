#pragma once

#include "Common/RPLM.Base.Framework.String.h"

namespace RPLM::CAD::DimensionChain::UI
{
	/// <summary>Сессия виджетов размерных расчетов</summary>
	class Session
	{
	private:
		Session();
		~Session();
	public:
		// Удаляем кострукторы копирования и перемещения
		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;
		Session(Session&&) = delete;
		Session& operator=(Session&&) = delete;

		static Session& Instance();

		// Инициализация
		void Init();

		// Завершение
		void Destroy();

		const RPLM::Base::Framework::String& GetModuleName();
	};
}
