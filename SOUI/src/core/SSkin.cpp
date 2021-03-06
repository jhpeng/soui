//////////////////////////////////////////////////////////////////////////
//   File Name: SSkinPool
//////////////////////////////////////////////////////////////////////////

#pragma once
#include "souistd.h"
#include "core/Sskin.h"
#include "helper/SDIBHelper.h"

namespace SOUI
{

//////////////////////////////////////////////////////////////////////////
// SSkinImgList
SSkinImgList::SSkinImgList()
:m_nStates(1)
,m_bTile(FALSE)
,m_bVertical(FALSE)
,m_pImg(NULL)
,m_filterLevel(kNone_FilterLevel)
,m_bAutoFit(TRUE)
{

}

SSkinImgList::~SSkinImgList()
{
    if(m_pImg) m_pImg->Release();
}

SIZE SSkinImgList::GetSkinSize()
{
    SIZE ret = {0, 0};
    if(m_pImg) ret=m_pImg->Size();
    if(m_bVertical) ret.cy/=m_nStates;
    else ret.cx/=m_nStates;
    return ret;
}

BOOL SSkinImgList::IgnoreState()
{
    return GetStates()==1;
}

int SSkinImgList::GetStates()
{
    return m_nStates;
}

void SSkinImgList::_Draw(IRenderTarget *pRT, LPCRECT rcDraw, DWORD dwState,BYTE byAlpha)
{
    if(!m_pImg) return;
    SIZE sz=GetSkinSize();
    RECT rcSrc={0,0,sz.cx,sz.cy};
    if(m_bVertical) 
        OffsetRect(&rcSrc,0, dwState * sz.cy);
    else
        OffsetRect(&rcSrc, dwState * sz.cx, 0);
    pRT->DrawBitmapEx(rcDraw,m_pImg,&rcSrc,GetExpandMode(),byAlpha);
}

UINT SSkinImgList::GetExpandMode()
{
    if(m_bAutoFit)
        return MAKELONG(m_bTile?EM_TILE:EM_STRETCH,m_filterLevel);
    else
        return MAKELONG(EM_NULL,m_filterLevel);
}

void SSkinImgList::OnColorize(COLORREF cr)
{
    if(!m_bEnableColorize) return;
    if(cr == m_crColorize) return;
	m_crColorize = cr;

    if(m_imgBackup)
    {//restore
        LPCVOID pSrc = m_imgBackup->GetPixelBits();
        LPVOID pDst = m_pImg->LockPixelBits();
        memcpy(pDst,pSrc,m_pImg->Width()*m_pImg->Height()*4);
        m_pImg->UnlockPixelBits(pDst);
    }else
    {
        if(S_OK != m_pImg->Clone(&m_imgBackup)) return;
    }
    
	if(cr!=0)
		SDIBHelper::Colorize(m_pImg,cr);
	else
		m_imgBackup = NULL;//free backup
}

//////////////////////////////////////////////////////////////////////////
//  SSkinImgFrame
SSkinImgFrame::SSkinImgFrame()
    : m_rcMargin(0,0,0,0)
{
}

void SSkinImgFrame::_Draw(IRenderTarget *pRT, LPCRECT rcDraw, DWORD dwState,BYTE byAlpha)
{
    if(!m_pImg) return;
    SIZE sz=GetSkinSize();
    CPoint pt;
    if(IsVertical())
        pt.y=sz.cy*dwState;
    else
        pt.x=sz.cx*dwState;
    CRect rcSour(pt,sz);
    pRT->DrawBitmap9Patch(rcDraw,m_pImg,&rcSour,&m_rcMargin,GetExpandMode(),byAlpha);
}

UINT SSkinImgFrame::GetExpandMode()
{
    return MAKELONG(m_bTile?EM_TILE:EM_STRETCH,m_filterLevel);
}

//////////////////////////////////////////////////////////////////////////
// SSkinButton
SSkinButton::SSkinButton()
    : m_nCornerRadius(2)
{
    m_colors.m_crBorder = RGB(0x70, 0x70, 0x70);
    m_colors.m_crUp[0]=(RGB(0xEE, 0xEE, 0xEE));
    m_colors.m_crDown[0]=(RGB(0xD6, 0xD6, 0xD6));
    m_colors.m_crUp[1]=(RGB(0xEE, 0xEE, 0xEE));
    m_colors.m_crDown[1]=(RGB(0xE0, 0xE0, 0xE0));
    m_colors.m_crUp[2]=(RGB(0xCE, 0xCE, 0xCE));
    m_colors.m_crDown[2]=(RGB(0xC0, 0xC0, 0xC0));
    m_colors.m_crUp[3]=(RGB(0x8E, 0x8E, 0x8E));
    m_colors.m_crDown[3]=(RGB(0x80, 0x80, 0x80));
}

void SSkinButton::_Draw(IRenderTarget *pRT, LPCRECT prcDraw, DWORD dwState,BYTE byAlpha)
{
    CRect rcDraw = *prcDraw;
    
    rcDraw.DeflateRect(1, 1);
    CAutoRefPtr<IRegion> rgnClip;
    if(m_nCornerRadius>2)
    {
        GETRENDERFACTORY->CreateRegion(&rgnClip);
        //the last two params of CreateRoundRectRgn are width and height of ellipse, thus we should multiple corner radius by 2.
        HRGN hRgn = ::CreateRoundRectRgn(prcDraw->left,prcDraw->top,prcDraw->right+1,prcDraw->bottom+1,m_nCornerRadius*2,m_nCornerRadius*2);
        rgnClip->SetRgn(hRgn);
        DeleteObject(hRgn);
    }
    if(rgnClip)
        pRT->PushClipRegion(rgnClip);
    if(m_colors.m_crUp[dwState]!=m_colors.m_crDown[dwState])
        pRT->GradientFill(rcDraw,TRUE,m_colors.m_crUp[dwState],m_colors.m_crDown[dwState],byAlpha);
    else
    {
        SColor cr(m_colors.m_crDown[dwState]);
        cr.updateAlpha(byAlpha);
        pRT->FillSolidRect(prcDraw,cr.toCOLORREF());
    }

    CAutoRefPtr<IPen> pPen,pOldPen;
    pRT->CreatePen(PS_SOLID,m_colors.m_crBorder,1,&pPen);
    pRT->SelectObject(pPen,(IRenderObj**)&pOldPen);
    pRT->DrawRoundRect(prcDraw,CPoint(m_nCornerRadius,m_nCornerRadius));
    pRT->SelectObject(pOldPen);
    
    if(rgnClip)
        pRT->PopClip();
}

BOOL SSkinButton::IgnoreState()
{
    return FALSE;
}

int SSkinButton::GetStates()
{
    return 4;
}

void SSkinButton::SetColors( COLORREF crUp[4],COLORREF crDown[4],COLORREF crBorder )
{
    memcpy(m_colors.m_crUp,crUp,4*sizeof(COLORREF));
    memcpy(m_colors.m_crDown,crDown,4*sizeof(COLORREF));
    m_colors.m_crBorder=crBorder;
}

void SSkinButton::OnColorize(COLORREF cr)
{
    if(!m_bEnableColorize) return;
    if(m_crColorize == cr) return;
    if(cr == 0)
    {
        memcpy(&m_colors,&m_colorsBackup,sizeof(BTNCOLORS));
        m_crColorize = 0;
    }else
    {
        if(m_crColorize!=0)
        {//从备份里获取数据
            memcpy(&m_colors,&m_colorsBackup,sizeof(BTNCOLORS));
        }else
        {//将数据备份
            memcpy(&m_colorsBackup,&m_colors,sizeof(BTNCOLORS));
        }
        m_crColorize = cr;

        //调整颜色值
        SDIBHelper::Colorize(m_colors.m_crBorder,m_crColorize);
        for(int i=0;i<4;i++)
        {
            SDIBHelper::Colorize(m_colors.m_crDown[i],m_crColorize);
            SDIBHelper::Colorize(m_colors.m_crUp[i],m_crColorize);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// SSkinGradation
SSkinGradation::SSkinGradation()
    : m_bVert(TRUE)
    , m_crFrom(CR_INVALID)
    , m_crTo(CR_INVALID)
    , m_crColorize(0)
{
}

void SSkinGradation::_Draw(IRenderTarget *pRT, LPCRECT prcDraw, DWORD dwState,BYTE byAlpha)
{
    pRT->GradientFill(prcDraw,m_bVert,m_crFrom,m_crTo,byAlpha);
}

void SSkinGradation::OnColorize(COLORREF cr)
{
    if(!m_bEnableColorize) return;
    if(m_crColorize == cr) return;
    if(m_crColorize!=0)
    {
        m_crFrom = m_crFromBackup;
        m_crTo = m_crToBackup;
    }else
    {
        m_crFromBackup = m_crFrom;
        m_crToBackup = m_crTo;
    }
    m_crColorize = cr;
    SDIBHelper::Colorize(m_crFrom,cr);
    SDIBHelper::Colorize(m_crTo,cr);
}

//////////////////////////////////////////////////////////////////////////
// SScrollbarSkin
SSkinScrollbar::SSkinScrollbar():m_nMargin(0),m_bHasGripper(FALSE),m_bHasInactive(FALSE)
{
    
}

CRect SSkinScrollbar::GetPartRect(int nSbCode, int nState,BOOL bVertical)
{
    CSize sz=GetSkinSize();
    CSize szFrame(sz.cx/9,sz.cx/9);
    if(nSbCode==SB_CORNOR)
    {
        return CRect(CPoint(szFrame.cx*8,0),szFrame);
    }else if(nSbCode==SB_THUMBGRIPPER)
    {
        return CRect(CPoint(szFrame.cx*8,(1+(bVertical?0:1))*szFrame.cy),szFrame);
    }else
    {
        if(nState==SBST_INACTIVE && !m_bHasInactive)
        {
            nState=SBST_NORMAL;
        }
        CRect rcRet;
        int iPart=-1;
        switch(nSbCode)
        {
        case SB_LINEUP:
            iPart=0;
            break;
        case SB_LINEDOWN:
            iPart=1;
            break;
        case SB_THUMBTRACK:
            iPart=2;
            break;
        case SB_PAGEUP:
        case SB_PAGEDOWN:
            iPart=3;
            break;
        }
        if(!bVertical) iPart+=4;
        
        return CRect(CPoint(szFrame.cx*iPart,szFrame.cy*nState),szFrame);
    }
}

void SSkinScrollbar::_Draw(IRenderTarget *pRT, LPCRECT prcDraw, DWORD dwState,BYTE byAlpha)
{
    if(!m_pImg) return;
    int nSbCode=LOWORD(dwState);
    int nState=LOBYTE(HIWORD(dwState));
    BOOL bVertical=HIBYTE(HIWORD(dwState));
    CRect rcMargin(0,0,0,0);
    if(bVertical)
        rcMargin.top=m_nMargin,rcMargin.bottom=m_nMargin;
    else
        rcMargin.left=m_nMargin,rcMargin.right=m_nMargin;

    CRect rcSour=GetPartRect(nSbCode,nState,bVertical);
    
    pRT->DrawBitmap9Patch(prcDraw,m_pImg,&rcSour,&rcMargin,m_bTile?EM_TILE:EM_STRETCH,byAlpha);
    
    if(nSbCode==SB_THUMBTRACK && m_bHasGripper)
    {
        rcSour=GetPartRect(SB_THUMBGRIPPER,0,bVertical);
        CRect rcDraw=*prcDraw;
        
        if (bVertical)
            rcDraw.top+=(rcDraw.Height()-rcSour.Height())/2,rcDraw.bottom=rcDraw.top+rcSour.Height();
        else
            rcDraw.left+=(rcDraw.Width()-rcSour.Width())/2,rcDraw.right=rcDraw.left+rcSour.Width();
        pRT->DrawBitmap9Patch(&rcDraw,m_pImg,&rcSour,&rcMargin,m_bTile?EM_TILE:EM_STRETCH,byAlpha);
    }
}


//////////////////////////////////////////////////////////////////////////
// SSkinColor
SSkinColorRect::SSkinColorRect():m_nRadius(0)
{
    m_crStates[0]=RGBA(255,255,255,255);
    m_crStates[1]=CR_INVALID;
    m_crStates[2]=CR_INVALID;
    m_crStates[3]=CR_INVALID;
}

SSkinColorRect::~SSkinColorRect()
{
}

void SSkinColorRect::_Draw(IRenderTarget *pRT, LPCRECT prcDraw, DWORD dwState,BYTE byAlpha)
{
    if(dwState>3) return;
    
    if(m_crStates[dwState]==CR_INVALID)
        dwState =0;
    SColor cr(m_crStates[dwState]);
    cr.updateAlpha(byAlpha);
    if(m_nRadius!=0)
        pRT->FillSolidRoundRect(prcDraw,CPoint(m_nRadius,m_nRadius),cr.toCOLORREF());
    else
        pRT->FillSolidRect(prcDraw,cr.toCOLORREF());
}

int SSkinColorRect::GetStates()
{
	int nStates = 4;
	for(int i=3;i>=0;i--)
	{
		if(m_crStates[i] == CR_INVALID) nStates--;
		else break;
	}
	return nStates;
}

}//namespace SOUI