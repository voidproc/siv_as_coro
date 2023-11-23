
int Func1()
{
	Print << "Func1";
	return 1;
}

void Coro1(Vec2 &state)
{
	Print << "コルーチン開始";

	Print << "渡された引数: " << state;
	Yield();

	state.moveBy(150, 50);
	Yield();

	state.moveBy(50, 100);
	Yield();

	state.moveBy(0, 150);
	Print << "コルーチン終了";
}
