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

		~ScriptCoroutine()
		{
			if (ctx_ != nullptr)
			{
				ctx_->Release();
			}
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

namespace ScriptFunctions
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
	}
}

void Main()
{
	Scene::SetBackground(Color{ 16 });

	ScriptFunctions::Binding::RegisterFunctions(Script::GetEngine());

	const CustomScript script{ U"coro.as" };

	// 通常の関数を呼んでみる
	//auto func = script.getFunction<int()>(U"Func1");
	//int funcRet = func();
	//Print << funcRet;

	// コルーチンを作成
	auto coro = script.getCoroutine<Vec2>(U"Coro1", Vec2{ 400, 200 });

	// ねこ
	const auto cat = Texture{ U"🐈"_emoji };

	while (System::Update())
	{
		if (SimpleGUI::ButtonAt(U"コルーチンを実行"_sv, Scene::CenterF()))
		{
			Print << U"▼";

			// コルーチンを呼ぶ
			coro();

			// コルーチンの状態
			Print << coro.getState();
		}

		cat.rotated(10_deg * Periodic::Sine1_1(2.4)).drawAt(coro.getState());
	}
}
