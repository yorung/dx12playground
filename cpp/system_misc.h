class SystemMisc
{
public:
	IVec2 screenSize;
	IVec2 mousePos;
	bool mouseDown;
	double lastUpdateTime;
	void SetScreenSize(IVec2 size) { screenSize = size; }
	const IVec2& GetScreenSize() { return screenSize; }
	void SetMousePos(IVec2 pos) { mousePos = pos; }
	const IVec2& GetMousePos() { return mousePos; }
	double GetLastUpdateTime() { return lastUpdateTime; }
};
extern SystemMisc systemMisc;
