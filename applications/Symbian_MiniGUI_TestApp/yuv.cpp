#include <w32std.h> 
#include "profiler.h"

#include <graphics/surfacemanager.h>
#include <graphics/surfaceupdateclient.h>
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
			// Semitransparent green or black
			gc->SetBrushColor(color? TRgb(0x00ff00, 30) : TRgb(0x000000, 30));
			gc->DrawRect(rect);
			}
	}
	
	// Open the surface manager
	RSurfaceManager surfaceManager;
	User::LeaveIfError(surfaceManager.Open());
	// Store the attributes used to create the Surface
	RSurfaceManager::TSurfaceCreationAttributesBuf buf;
	RSurfaceManager::TSurfaceCreationAttributes& attributes = buf();

	attributes.iSize = TSize(320, 240);      // w > 0, h > 0
	attributes.iBuffers = 4;                // > 0, <= 4
	attributes.iPixelFormat = EUidPixelFormatYUV_422Interleaved; // 1bpp
	attributes.iStride = 320;              // > 0,  width * bpp
	attributes.iOffsetToFirstBuffer = 184;  // > 0, divisible by alignment
	attributes.iAlignment = 4;              // 1 || 2 || 4 || 8
	attributes.iContiguous = EFalse;
	attributes.iMappable = ETrue;

	RSurfaceManager::THintPair hints[2];         // two hint pairs specified
	attributes.iHintCount=2;
	hints[0].Set(TUid::Uid(0x124578), 20, ETrue);
	hints[1].Set(TUid::Uid(0x237755), 50, EFalse);
	attributes.iSurfaceHints = hints;

	attributes.iOffsetBetweenBuffers = 0;
	// Create a surface
	TSurfaceId surfaceId;
	User::LeaveIfError(surfaceManager.CreateSurface(buf, surfaceId));

	// We have a surface, so map it in.
	RChunk chunk;
	TInt err = surfaceManager.MapSurface(surfaceId, chunk);
	if ( err == KErrNone)
	    { 
	    // Get info about it
	    RSurfaceManager::TInfoBuf buf;
	    RSurfaceManager::TSurfaceInfoV01& surfaceInfo = buf(); 

	    surfaceManager.SurfaceInfo(surfaceId, buf); 
	    }

	
	
	win.SetBackgroundSurface(surfaceId);
	gc->Deactivate();
	win.EndRedraw();
	ws.Flush();
 	User::After(3000000);
 	
 	win.RemoveBackgroundSurface(ETrue);
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
