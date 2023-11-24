
void CatStateTest(CatState& state)
{
	Print << state.pos;
	Print << state.time.sF();
	state.time.start();
	Yield();

	state.pos = RandomVec2();
	Print << state.pos;
	Print << state.time.sF();
	Yield();
}

void UpdateCat(CatState& state)
{
	// 画面上のある点に向かう * 3
	for (int i = 0; i < 3; ++i)
	{
		const Vec2 posStart = state.pos;
		const Vec2 posEnd = RandomVec2(Scene::Rect().stretched(-32));
		const double period = Random(1.0, 3.0);
		Stopwatch time(StartImmediately::Yes);
		while (time.sF() < period)
		{
			const double time0_1 = Clamp(time.sF() / period, 0.0, 1.0);
			state.pos = posStart.lerp(posEnd, EaseInOutSine(time0_1));
			Yield();
		}
	}

	// 画面外へ
	{
		const Vec2 posStart = state.pos;
		const Vec2 posEnd = Vec2(state.pos.x, state.pos.y - Scene::Height() - 64);
		const double period = Random(2.0, 5.0);
		Stopwatch time(StartImmediately::Yes);
		while (time.sF() < period)
		{
			const double time0_1 = Clamp(time.sF() / period, 0.0, 1.0);
			state.pos = posStart.lerp(posEnd, EaseInOutSine(time0_1));
			Yield();
		}
	}
}
