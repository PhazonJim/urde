#include "CSlideShow.hpp"
#include "GameGlobalObjects.hpp"
#include "Editor/ProjectManager.hpp"

namespace urde
{

CSlideShow::CSlideShow()
: CIOWin("SlideShow"), x5c_slideA(*this), x90_slideB(*this)
{
    x130_ = g_tweakSlideShow->GetX54();
    x134_24_ = true;
    x134_30_ = true;
    x135_24_ = true;

    const SObjectTag* font = g_ResFactory->GetResourceIdByName(g_tweakSlideShow->GetFont().c_str());
    if (font)
    {
        CGuiTextProperties propsA(false, true, EJustification::Center, EVerticalJustification::Bottom);
        xc4_textA = std::make_unique<CGuiTextSupport>(font->id, propsA,
                                                      g_tweakSlideShow->GetFontColor(),
                                                      g_tweakSlideShow->GetOutlineColor(),
                                                      zeus::CColor::skWhite, 640, 480, g_SimplePool);

        CGuiTextProperties propsB(false, true, EJustification::Right, EVerticalJustification::Bottom);
        xc8_textB = std::make_unique<CGuiTextSupport>(font->id, propsB,
                                                      g_tweakSlideShow->GetFontColor(),
                                                      g_tweakSlideShow->GetOutlineColor(),
                                                      zeus::CColor::skWhite, 640, 480, g_SimplePool);
    }

    xf8_stickTextures.reserve(18);
    x108_buttonTextures.reserve(8);
    SObjectTag txtrTag(FOURCC('TXTR'), 0);
    for (int i=0 ; i<9 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->x24_lStick[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
    for (int i=0 ; i<9 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->x4c_cStick[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
    for (int i=0 ; i<2 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->x74_lTrigger[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
    for (int i=0 ; i<2 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->x80_rTrigger[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
    for (int i=0 ; i<2 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->xa4_bButton[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
    for (int i=0 ; i<2 ; ++i)
    {
        txtrTag.id = g_tweakPlayerRes->xbc_yButton[i];
        xf8_stickTextures.push_back(g_SimplePool->GetObj(txtrTag));
    }
}

bool CSlideShow::LoadTXTRDep(const std::string& name)
{
    const SObjectTag* dgrpTag = g_ResFactory->GetResourceIdByName(name.c_str());
    if (dgrpTag && dgrpTag->type == FOURCC('DGRP'))
    {
        x18_galleryTXTRDeps.push_back(g_SimplePool->GetObj(*dgrpTag));
        return true;
    }
    return false;
}

bool CSlideShow::AreAllDepsLoaded(const std::vector<TLockedToken<CDependencyGroup>>& deps)
{
    for (const TLockedToken<CDependencyGroup>& token : deps)
    {
        if (!token.IsLoaded())
            return false;
    }
    return true;
}

CIOWin::EMessageReturn CSlideShow::OnMessage(const CArchitectureMessage& msg, CArchitectureQueue& queue)
{
    switch (msg.GetType())
    {
    case EArchMsgType::TimerTick:
    {
        if (x134_29_)
            return EMessageReturn::RemoveIOWinAndExit;

        float dt = MakeMsg::GetParmTimerTick(msg).x4_parm;

        switch (x14_phase)
        {
        case Phase::Zero:
        {
            //if (!g_resLoader->AreAllPaksLoaded())
            //{
            //    g_resLoader->AsyncIdlePakLoading();
            //    return EMessageReturn::Exit;
            //}
            x14_phase = Phase::One;
        }
        case Phase::One:
        {
            if (x18_galleryTXTRDeps.empty())
            {
                x18_galleryTXTRDeps.reserve(5);
                for (int i=1 ; true ; ++i)
                {
                    std::string depResName = hecl::Format("Gallery%02d_DGRP", i);
                    if (!LoadTXTRDep(depResName))
                        break;
                }
                LoadTXTRDep("GalleryAssets_DGRP");
            }
            if (!AreAllDepsLoaded(x18_galleryTXTRDeps))
                return EMessageReturn::Exit;

            x14_phase = Phase::Three;
        }
        case Phase::Two:
        case Phase::Three:
        {

        }
        case Phase::Four:
        case Phase::Five:
        default: break;
        }

        break;
    }
    case EArchMsgType::UserInput:
    default: break;
    }

    return EMessageReturn::Exit;
}

void CSlideShow::SSlideData::Draw() const
{
    if (!IsLoaded())
        return;

    zeus::CRectangle rect;
    const_cast<CTexturedQuadFilterAlpha&>(*m_texQuad).draw(x30_mulColor, 1.f, rect);

    zeus::CVector2f centeredOffset((x28_canvasSize.x - m_texQuad->GetTex()->GetWidth()) * 0.5f,
                                   (x28_canvasSize.y - m_texQuad->GetTex()->GetHeight()) * 0.5f);
}

void CSlideShow::Draw() const
{
    if (x14_phase == Phase::Five)
    {
        x5c_slideA.Draw();
        x90_slideB.Draw();
    }
}

}