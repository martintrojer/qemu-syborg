#include <w32std.h> 
#include "profiler.h"

const TSize KSize(64,60);
const TInt KRow = 4;
const TInt KCol = 5; 

void MainL()	
	{	
	RWsSession ws;
	ws.Connect();
 	CWsScreenDevice* scr = new(ELeave) CWsScreenDevice(ws);
	scr->Construct();
 	CWindowGc* gc = new(ELeave) CWindowGc(scr);
	gc->Construct();
 	RWindowGroup grp(ws);
	grp.Construct(0xc0decafe, ETrue);
 	RWindow win(ws);
	win.Construct(grp, 0xbeefcafe);
	win.SetExtent(TPoint(20,160), TSize(320,240));
	win.Activate();
 	win.Invalidate();
	win.BeginRedraw();
	gc->Activate(win);
 	gc->SetPenStyle(CGraphicsContext::ENullPen);
	gc->SetBrushStyle(CGraphicsContext::ESolidBrush);
 	TBool color = EFalse;

if (Profiler::Start() == KErrNotFound)
	{
	_LIT(KProfiler,"profiler");
	_LIT(KStart,"start -noui -drive=S");
	RProcess p;
	if (p.Create(KProfiler,KStart) == KErrNone)
		{
		p.Resume();
		p.Close();
		}
	}

	for (TInt col=0; col<KCol; ++col)
		{
		color = !color;		
		for (TInt row=0; row<KRow; ++row)
			{
			TRect rect;
			rect.iTl.iX = col * KSize.iWidth;
			rect.iTl.iY = row * KSize.iHeight;
			rect.SetSize(KSize);
			color = !color;
			gc->SetBrushColor(color? KRgbBlue : KRgbBlack);
			gc->DrawRect(rect);
			}
	}
	
	
	
	gc->Deactivate();
	win.EndRedraw();
	ws.Flush();
 	User::After(3000000);
 	win.Close();
	grp.Close();
	delete gc;
	delete scr;
	ws.Close();

	Profiler::Stop();
	Profiler::Close();
	Profiler::Unload();

	} 


GLDEF_C TInt E32Main()	
{	

	CTrapCleanup* tc = CTrapCleanup::New();	
	if (!tc)
		{		
		return KErrNoMemory;
		}
	TRAPD(err, MainL());
	delete tc;
 	return err;	
}
