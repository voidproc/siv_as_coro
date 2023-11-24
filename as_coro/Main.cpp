# include <Siv3D.hpp> // Siv3D v0.6.13

namespace s3d
{
	using namespace AngelScript;

	/// @brief AngelScriptのコルーチン
	///
	/// AngelScriptのコルーチンはサスペンド時に値を返すことができないので、
	/// 値をやり取りするための変数(state_)のポインタをコルーチン作成時に渡す。
	/// スクリプト内部で書き換えられた値を getState() で得ることができる。
	/// 
	/// @tparam State コルーチンに渡す引数の型
	template <class State>
	class ScriptCoroutine
	{
	public:
		ScriptCoroutine(asIScriptContext* ctx = nullptr, const State& initialState = State{})
			: ctx_{ ctx }, state_{ initialState }
		{
			ctx_->SetArgAddress(0, &state_);
		}

		ScriptCoroutine(const ScriptCoroutine&) = delete;

		ScriptCoroutine(ScriptCoroutine&& sc)
			: ScriptCoroutine{ sc.ctx_, sc.state_ }
		{
			sc.ctx_ = nullptr;
		}

		~ScriptCoroutine()
		{
			if (ctx_ != nullptr)
			{
				ctx_->Release();
			}
		}

		ScriptCoroutine& operator =(const ScriptCoroutine&) = delete;

		ScriptCoroutine& operator =(ScriptCoroutine&& sc)
		{
			ctx_ = sc.ctx_;
			sc.ctx_ = nullptr;
			state_ = sc.state_;
		}

		/// @brief コルーチンが有効なら実行する
		void operator ()() const
		{
			if (runnable())
			{
				ctx_->Execute();
			}
		}

		/// @brief コルーチンが有効か
		bool runnable() const
		{
			if (ctx_ == nullptr) return false;

			const auto state = ctx_->GetState();

			return (
				state == asEContextState::asEXECUTION_PREPARED ||
				state == asEContextState::asEXECUTION_SUSPENDED);
		}

		asIScriptContext* getContext() const
		{
			return ctx_;
		}

		State& getState()
		{
			return state_;
		}

		const State& getState() const
		{
			return state_;
		}

	private:
		asIScriptContext* ctx_;
		State state_;
	};

	/// @brief s3d::Script に getCoroutine() を追加したもの
	class CustomScript : public Script
	{
	public:
		SIV3D_NODISCARD_CXX20
		explicit CustomScript(FilePathView path, ScriptCompileOption compileOption = ScriptCompileOption::Default)
			: Script(path, compileOption)
		{
		}

		/// @brief コルーチンを作成する
		/// @tparam CoroState コルーチンに渡す引数の型
		/// @param decl 関数名
		/// @param initialState コルーチンに渡す引数の値
		template <class CoroState>
		ScriptCoroutine<CoroState> getCoroutine(StringView decl, const CoroState& initialState = CoroState{}) const
		{
			return ScriptCoroutine<CoroState>{ getCoroutineContext_(decl), initialState };
		}

	private:
		asIScriptContext* getCoroutineContext_(StringView decl) const
		{
			// https://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_coroutine.html

			if (isEmpty())
			{
				return nullptr;
			}

			asIScriptModule* mod = _getModule()->module;

			asIScriptFunction* funcPtr = mod->GetFunctionByName(decl.narrow().c_str());

			if (funcPtr == nullptr)
			{
				return nullptr;
			}

			// コルーチン用のContextを作成
			asIScriptContext* coctx = GetEngine()->CreateContext();
			coctx->Prepare(funcPtr);

			return coctx;
		}
	};
}

struct CatState
{
	Vec2 pos{};
	Stopwatch time{};
};

namespace Scripting
{
	using namespace AngelScript;

	namespace Binding
	{
		/// @brief コルーチンを一時停止する
		static void Yield()
		{
			if (asIScriptContext* ctx = asGetActiveContext();
				ctx)
			{
				ctx->Suspend();
			}
		}

		static void RegisterFunctions(asIScriptEngine* engine)
		{
			engine->RegisterGlobalFunction("void Yield()", asFUNCTION(Yield), asCALL_CDECL);
		}

		static void RegisterObjects(asIScriptEngine* engine)
		{
			engine->RegisterObjectType("CatState", sizeof(CatState), asOBJ_VALUE | asOBJ_POD);
			engine->RegisterObjectProperty("CatState", "Vec2 pos", 0);
			engine->RegisterObjectProperty("CatState", "Stopwatch time", 16);
		}
	}
}

void Main()
{
	Scene::SetBackground(Palette::Chocolate.lerp(Palette::Black, 0.5));

	Scripting::Binding::RegisterFunctions(Script::GetEngine());
	Scripting::Binding::RegisterObjects(Script::GetEngine());

	const CustomScript script{ U"coro.as" };

	// コルーチンたち
	using Coro = ScriptCoroutine<CatState>;
	Array<std::shared_ptr<Coro>> coroList{ Arg::reserve = 256 };

	// コルーチン作成用タイマー
	Timer timerMakeCoro{ 0.2s, StartImmediately::Yes };

	// ねこ
	const auto cat = Texture{ U"🐱"_emoji };

	while (System::Update())
	{
		if (timerMakeCoro.reachedZero())
		{
			timerMakeCoro.restart();

			for (auto i : step(Random(2, 5)))
			{
				coroList.emplace_back(std::make_shared<Coro>(script.getCoroutine<CatState>(U"UpdateCat", CatState{ RandomVec2(Scene::Rect().bottom().movedBy(0, 80)), Stopwatch{ StartImmediately::Yes } })));
			}
		}

		for (auto& coro : coroList)
		{
			(*coro)();

			cat.scaled(0.75).rotated(10_deg * Periodic::Sine1_1(2.2s, coro->getState().time.sF())).drawAt(coro->getState().pos, ColorF{ 0, 0.5 });
			cat.scaled(0.7).rotated(10_deg * Periodic::Sine1_1(2.2s, coro->getState().time.sF())).drawAt(coro->getState().pos);
		}

		coroList.remove_if([](const auto& coro) { return not coro->getState().pos.intersects(Scene::Rect().stretched(100)); });

		PutText(Format(coroList.size()), Arg::topLeft = Vec2{ 16, 16 });
	}
}
