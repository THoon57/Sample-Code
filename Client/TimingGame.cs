using DEFINE;
using NGraphics;
using NLibCs;
using PROTOCOL.FLATBUFFERS;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using static DEFINE.GameDefine;
using static NGraphicEnums;

public class IntoTheFireDlg : UIForm
{
#if UNITY_EDITOR
    [UnityEditor.MenuItem("NDREAM/DlgTest/IntoTheFireDlg")]
    public static void Test_IntoTheFireDlg()
    {
        var dlg = UIFormManager.Instance.OpenUIForm<IntoTheFireDlg>();
    }
#endif

    private float m_ArrowSpeed = 1f;

    private const float MIN_GUAGE = -325f;
    private const float MAX_GUAGE = 325f;
    private const float MIN_SUCCESS_GUAGE = -210f;
    private const float MAX_SUCCESS_GUAGE = 210f;
    private const float MIN_GREAT_SUCCESS_GUAGE = -73.5f;
    private const float MAX_GREAT_SUCCESS_GUAGE = 73.5f;
    private Int64 REMAIN_TIME = 20;

    private bool Moving = false;
    private bool bCountDown = false;
    private bool bProgress = false;


    private UIButton Button_Return = null;
    private UIButton Button_Shot = null;
    private UILabel Label_Wave = null;
    private UILabel Label_Remain = null;
    private UILabel Label_Time = null;
    private UILabel Label_Bonus = null;
    private UILabel Label_Point = null;
    private UILabel Label_Summary = null;

    private GameObject Guage = null;
    private Animator Ani_Guage = null;
    private Animator Countdown = null;
    private Animator Ani_Progress = null;
    private Animator Ani_Failed = null;
    private Animator Ani_Success = null;
    private Animator Ani_Perfect = null;

    private UISprite Sprite_Arrow = null;
    private UISprite Sprite_GuageBack = null;
    private UISprite Sprite_Midguage = null;
    private UISprite Sprite_Foreguage = null;

    private TweenScale Tween_Point = null;
    private List<Transform> List_Enemys = new List<Transform>();
    private Camera Camera_IntotheFire = null;

    private Coroutine MoveArrow = null;

    private Transform FX_Point = null;
    private GameObject Point_Effect = null;

    private EnumSceneState curScene;

    private CustomEventData m_Data = null;
    private int m_curEnemyCount = 0;
    private eCUSTUM_EVENT_DIFFICULTY m_Difficulty = eCUSTUM_EVENT_DIFFICULTY.NONE;
    private int m_bounsPoint = 0;
    private int m_AniProgressNum = 0;
    private List<Item> m_RewardItems = new List<Item>();
    bool m_Shooting = false;
    bool m_CheckPoint = false;

    private EVENT.DummyEventDirection m_dummyEvent = null;
    public override void BindUIControls()
    {
        base.BindUIControls();
        base.ApplyBoundGameObjectGroup();
    }

    public override void BindUIEvents()
    {
        base.BindUIEvents();

        if (Button_Return != null)
            BindEvent(Button_Return, OnClick_Exit);

        if (Button_Shot != null)
            BindEvent(Button_Shot, OnClick_Shoot);
    }



    public override void Close()
    {
        if (CloseReason == ReasonType.Escape)
        {
            OnClick_Exit();
            return;
        }
        base.Close();

        InputBlockDlg.Block("IntoTheFireDlg", false);
        //아이콘
        PublicUIMethod.Active_HeadUpIcon(true);
        //사운드?
        TerritoryController.Instance.PlaySound_GroundCliff(true);
        //메인 UI
        UIFormManager.Instance.FixedHideAniMainUI(false);

        if (m_dummyEvent != null)
        {
            m_dummyEvent.End();
            m_dummyEvent = null;
        }

        //그림자 옵션.
        NGraphic.Instance.SetSceneState((curScene), 0);

        AudioManager.Instance.StopAMB("amb_battle04", 0.0f, 1.0f);

        if (GameMain.IsTerritoryStage == true)
            AudioManager.Instance.PlayBGM("bgm_territory");
        else
            AudioManager.Instance.PlayBGM("bgm_field");

        if (IsInvoking(nameof(UpdateRemainTime)))
            CancelInvoke(nameof(UpdateRemainTime));

        GameCameraController.Instance.SetCurComponent(GameMain.IsTerritoryStage ? GameCameraController.CamComponentType.Territory : GameCameraController.CamComponentType.Field);

        StopAllCoroutines();
    }

    public override void Open()
    {
        base.Open();

        m_Data = CustomEventManager.Instance.GetCurrentEvent(GameDefine.eCUSTUM_EVENT_TYPE.TIMINGGAME);
        if (m_Data == null)
            return;

        m_curEnemyCount = CustomEventManager.Instance.GetCurEnemyCount(m_Data.EventID, m_Difficulty);
        Label_Time.Ex_SetText(REMAIN_TIME.ToString());
        Label_Point.Ex_SetText(m_bounsPoint.ToString());

        EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(m_dummyEvent = new EVENT.DummyEventDirection());

        SetUI();
        Setbar();

        //터치
        InputBlockDlg.Block("IntoTheFireDlg", true);
        //카메라
        GameCameraController.Instance.allCameraHide();
        //사운드
        TerritoryController.Instance.PlaySound_GroundCliff(false);
        //메인 UI
        UIFormManager.Instance.FixedHideAniMainUI(true);

        PublicUIMethod.Active_HeadUpIcon(false);

        AudioManager.Instance.PlayAMB("amb_battle04", 1f, 0f, 1f);
        AudioManager.Instance.PlayBGM("bgm_timingame");

        //그림자 옵션
        curScene = NGraphic.currentSceneState;
        NGraphic.Instance.SetSceneState((EnumSceneState.INTO_THEFIRE), 0);
        QualitySettings.shadowDistance = 40f;
                
        if (MoveArrow != null)
        {
            StopCoroutine(MoveArrow);
            MoveArrow = null;
        }

        Guage.Ex_SetActive(false);
        
        Button_Shot.Ex_SetActive(false);

        StartCoroutine(Co_StartCountDown());
    }

    public override void Init()
    {
        base.Init();
    }

    public override void Refresh()
    {
        base.Refresh();

        m_Data = CustomEventManager.Instance.GetCurrentEvent(GameDefine.eCUSTUM_EVENT_TYPE.TIMINGGAME);
        if (m_Data == null)
            return;

        SetUI();
        Setbar();
    }
    public void Setbar()
    {
        if (m_Data == null)
            return;

        if (m_Data is TimingGameEventData timingGameData)
        {
            var successBarSize = CustomEventManager.Instance.GetSuccessBarSize(m_Data.EventID, m_Difficulty);
            var greatSuccessBarSize = CustomEventManager.Instance.GetGreatSuccessBarSize(m_Data.EventID, m_Difficulty);
            Sprite_Midguage.SetDimensions((int)(Sprite_GuageBack.localSize.x * (successBarSize * 0.0001f)), (int)Sprite_Midguage.localSize.y);
            Sprite_Foreguage.SetDimensions((int)(Sprite_GuageBack.localSize.x * (greatSuccessBarSize * 0.0001f)), (int)Sprite_Midguage.localSize.y);

            var succassbarRatio = timingGameData.SucessBar;
            var greatSuccessBarRatio = timingGameData.GreatSucessBar;

            var halfMainBar = Sprite_GuageBack.localSize.x * 0.5f;
            var successbarPosX = Mathf.Lerp(-halfMainBar, halfMainBar, succassbarRatio * 0.0001f);
            var greatSuccessbarPosX = Mathf.Lerp(-halfMainBar, halfMainBar, greatSuccessBarRatio * 0.0001f);


            Sprite_Midguage.transform.localPosition = new Vector3(successbarPosX, Sprite_Midguage.transform.localPosition.y, Sprite_Midguage.transform.localPosition.z);
            Sprite_Foreguage.transform.localPosition = new Vector3(greatSuccessbarPosX, Sprite_Foreguage.transform.localPosition.y, Sprite_Foreguage.transform.localPosition.z);

        }


    }
    public void SetCurRound(int round)
    {
        m_curEnemyCount = CustomEventManager.Instance.GetCurEnemyCount(m_Data.EventID, m_Difficulty) - round;
    }

    public void SetRewardItems(List<Item> items)
    {
        m_RewardItems = items;
    }

    public List<Item> GetRewardItems()
    {
        return m_RewardItems;
    }

    public void AddBonusItemCount(int addItem)
    {
        if (m_Data == null)
            return;

        var valueData = CustomEventManager.Instance.GetValueData(m_Data.EventID, m_Difficulty);
        if (valueData == null)
            return;

        m_bounsPoint += addItem;
        m_CheckPoint = true;

        if (addItem == valueData.Value1)
            Ani_Failed.Ex_SetActive(true);
        else if (addItem == valueData.Value2)
            Ani_Success.Ex_SetActive(true);
        else if (addItem == valueData.Value3)
            Ani_Perfect.Ex_SetActive(true);

        MakeParticle((short)addItem);
    }


    public void SetUI()
    {
        Label_Remain.Ex_SetText(NTextManager.Instance.GetText("UI_TIMINGGAME_TIMELIMIT_TEXT"));
        Label_Wave.Ex_SetText(string.Format(NTextManager.Instance.GetText("UI_TIMINGGAME_SHOTNUM_TEXT"), m_curEnemyCount, CustomEventManager.Instance.GetCurEnemyCount(m_Data.EventID, m_Difficulty)));
        Label_Bonus.Ex_SetText(NTextManager.Instance.GetText("UI_TIMINGGAME_REWARDBONUS_TEXT"));
        Label_Summary.Ex_SetText(NTextManager.Instance.GetText("UI_TIMINGGAME_RESULT_DESC"));
    }
    IEnumerator Co_StartCountDown()
    {
        if (Countdown == null)
            yield break;

        if (bCountDown == false)
        {
            Countdown.Ex_SetActive(true);
            bCountDown = true;
        }

        while (Countdown.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            yield return null;
        }

        StartMoveArrow();

        if (IsInvoking(nameof(UpdateRemainTime)))
            CancelInvoke(nameof(UpdateRemainTime));

        InvokeRepeating(nameof(UpdateRemainTime), 0f, 1f);
    }

    IEnumerator Co_MoveArrow()
    {
        if(m_Data == null)
        {
            NDebug.Log("m_Data NULL");
            yield break;
        }

        var GetArrowSpeedTime = CustomEventManager.Instance.GetArrowSpeedTime(m_Data.EventID, m_Difficulty) * 0.01f;
        //왼쪽으로 가고 있었으면..
        if (m_ArrowSpeed < 0)
            GetArrowSpeedTime *= -1;

        m_ArrowSpeed = Sprite_GuageBack.localSize.x / GetArrowSpeedTime;
        while (Moving)
        {
            float arrowSpeed = m_ArrowSpeed * Time.deltaTime;
            Sprite_Arrow.transform.localPosition += new Vector3(arrowSpeed, 0, 0);

            if (Mathf.Approximately(Sprite_Arrow.transform.localPosition.x, MAX_GUAGE) || Sprite_Arrow.transform.localPosition.x >= MAX_GUAGE)
            {
                m_ArrowSpeed *= -1;
                Sprite_Arrow.transform.localPosition = new Vector3(MAX_GUAGE, Sprite_Arrow.transform.localPosition.y, Sprite_Arrow.transform.localPosition.z);
                yield return null;
            }

            if (Mathf.Approximately(Sprite_Arrow.transform.localPosition.x, MIN_GUAGE) || Sprite_Arrow.transform.localPosition.x <= MIN_GUAGE)
            {
                m_ArrowSpeed *= -1;
                Sprite_Arrow.transform.localPosition = new Vector3(MIN_GUAGE, Sprite_Arrow.transform.localPosition.y, Sprite_Arrow.transform.localPosition.z);
                yield return null;
            }
            yield return null;
        }

        yield break;
    }
    public void StopMoveArrow()
    {
        Moving = false;

        if (MoveArrow != null)
            StopCoroutine(MoveArrow);

    }
    public void StartMoveArrow()
    {
        if (MoveArrow != null)
            StopCoroutine(MoveArrow);

        Guage.Ex_SetActive(true);
        Button_Shot.Ex_SetActive(true);

        //초기 위치 맨 외쪽
        var halfSize = Sprite_GuageBack.localSize.x;
        Sprite_Arrow.transform.localPosition = new Vector3(-halfSize, Sprite_Arrow.transform.localPosition.y, Sprite_Arrow.transform.localPosition.z);

        //Hit Ani도 수동으로 꺼지게 바뀜..
        Ani_Failed.Ex_SetActive(false);
        Ani_Success.Ex_SetActive(false);
        Ani_Perfect.Ex_SetActive(false);
        
        Moving = true;

        if (m_CheckPoint == true)
        {
            Label_Point.Ex_SetText(m_bounsPoint.ToString());
            m_CheckPoint = false;
        }

        MoveArrow = StartCoroutine(Co_MoveArrow());
    }

    public void SetDifficulty(eCUSTUM_EVENT_DIFFICULTY difficulty)
    {
        m_Difficulty = difficulty;
    }

    private void UpdateRemainTime()
    {
        if (m_Data == null)
            return;


        if (REMAIN_TIME <= 0)
        {
            //StopAllCoroutines();
            Label_Time.Ex_SetText(REMAIN_TIME.ToString());
            if (IsInvoking(nameof(UpdateRemainTime)))
                CancelInvoke(nameof(UpdateRemainTime));

            Guage.Ex_SetActive(false);
            Button_Shot.Ex_SetActive(false);

            if(m_Shooting == false)
            {
                // 현재 정보로 결산..
                FlatBuffers.FlatBufferBuilder fbb = FlatBuffers.NFlatBufferBuilder.FBB;
                System.Func<object> OffsetMethod = () =>
                {

                    return PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ.CreateGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(fbb, m_Data.EventID, (Int16)m_Difficulty, 0, 0, true);
                };
                FlatBuffers.NFlatBufferBuilder.SendBytes<PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ>(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ, OffsetMethod);
            }
            else
            {
                StartCoroutine(Co_WaitSendResult());
            }
            
        }

        Label_Time.Ex_SetText(REMAIN_TIME.ToString());

        if (REMAIN_TIME > 0)
            REMAIN_TIME -= 1;
        else
            REMAIN_TIME = 0;
    }

    public void OnClick_Exit()
    {
        PublicSoundMethod.PlayClickCommon();


        IntotheFirePopupDlg messageBoxDlg = UIFormManager.Instance.OpenUIForm<IntotheFirePopupDlg>();
        messageBoxDlg.Set(NTextManager.Instance.GetText("UI_TIMINGGAME_EXIT_TEXT"),
            NTextManager.Instance.GetText("UI_TIMINGGAME_EXIT_DESC"),
            NTextManager.Instance.GetText("UI_COMMON_CONFIRM_CLOSE"),
            ()=> { var dlg = UIFormManager.Instance.FindUIForm<IntoTheFireDlg>(); if (dlg != null) dlg.Close(); },
            NTextManager.Instance.GetText("UI_COMMON_CONFIRM_NO")
            );
        messageBoxDlg._closeFunc = ReStartGame;

        //초기 연출이 진행중이라면 꺼준다.
        if (Countdown.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            Countdown.enabled = false;
        }
        else
        {
            if (IsInvoking(nameof(UpdateRemainTime)))
                CancelInvoke(nameof(UpdateRemainTime));

            StopMoveArrow();
        }
        
       
    }
    public void ReStartGame()
    {
        if (Countdown == null)
            return;

        if (Countdown.enabled == false)
            Countdown.enabled = true;
        else
        {
            InvokeRepeating(nameof(UpdateRemainTime), 0f, 1f);
            StartMoveArrow();
        }
    }
    public void OnClick_Shoot()
    {
        if (m_Data == null)
            return;

        //화살표 멈추고
        StopMoveArrow();
        //게이지 끄고
        StartCoroutine(Co_StartGuageOff());
        Button_Shot.Ex_SetActive(false);
        //각종 친구들 증감 해주고
        m_AniProgressNum++;
        m_Shooting = true;

        //연출 재생
        StartCoroutine(Co_StartProgress());

        //패킷 쏘기(랜덤 값 받기)
        FlatBuffers.FlatBufferBuilder fbb = FlatBuffers.NFlatBufferBuilder.FBB;
        System.Func<object> OffsetMethod = () =>
        {
            var halfMainBar = Sprite_GuageBack.localSize.x * 0.5f;
            var arrowPosRatioX = Sprite_Arrow.transform.localPosition.x + halfMainBar;
            var ratioArrowPosX = Mathf.Lerp(1, 10000, (arrowPosRatioX / Sprite_GuageBack.localSize.x));
            return PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ.CreateGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(fbb, m_Data.EventID, (Int16)m_Difficulty, m_AniProgressNum, Convert.ToInt32(ratioArrowPosX), false);
        };
        FlatBuffers.NFlatBufferBuilder.SendBytes<PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ>(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ, OffsetMethod);

    }
    IEnumerator Co_WaitSendResult()
    {
        while (m_Shooting)
            yield return null;

        FlatBuffers.FlatBufferBuilder fbb = FlatBuffers.NFlatBufferBuilder.FBB;
        System.Func<object> OffsetMethod = () =>
        {

            return PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ.CreateGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(fbb, m_Data.EventID, (Int16)m_Difficulty, 0, 0, true);
        };
        FlatBuffers.NFlatBufferBuilder.SendBytes<PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ>(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ, OffsetMethod);
    }

    public void MakeParticle(short particleCount)
    {
        if (particleCount <= 0)
            return;

        List<ObtainItemBase> paricles = new List<ObtainItemBase>();

        var enemyIndex = m_AniProgressNum - 1;
        if (List_Enemys.Count < enemyIndex || enemyIndex < 0)
            return;

        var screenPos = Camera_IntotheFire.WorldToScreenPoint(List_Enemys[enemyIndex].position);
        screenPos.x -= (Screen.width * 0.5f);
        screenPos.y -= (Screen.height * 0.5f);
        var viewPos = Camera_IntotheFire.ScreenToViewportPoint(screenPos);
        viewPos.z = 0f;

        var particleData = new ObtainItemBase(
            ObtainItemBase.ObtainItemType.NOITEM,
            particleCount,
            viewPos,
            Label_Point.transform.position,
            "fx_ui_87a_s1t_01",
            PlayTween);

        paricles.Add(particleData);
        ObtainItem_ControllerEx.EnQueueEvent(paricles, ObtainEventEx.ContentsType.CONTENTS_COMMON);
    }

    IEnumerator Co_StartGuageOff()
    {
        if (Ani_Guage == null)
            yield break;

        Ani_Guage.Rebind();
        Ani_Guage.Play("IntoTheFireDlg_Gauge_Off");

        while (Ani_Guage.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            yield return null;
        }

        Guage.Ex_SetActive(false);
    }

    IEnumerator Co_StartProgress()
    {
        if (Ani_Progress == null)
            yield break;

        if (bProgress == false)
        {
            Ani_Progress.Rebind();
            Ani_Progress.Play(string.Format("IntoTheFireDlg_Stage_{0:D2}", m_AniProgressNum));
            bProgress = true;
        }

        while (Ani_Progress.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            yield return null;
        }


        bProgress = false;
    }
    IEnumerator Co_WaitStartMoveArrow()
    {
        if (Ani_Progress == null)
            yield break;

        while (Ani_Progress.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            yield return null;
        }

        m_Shooting = false;
        if(REMAIN_TIME > 0)
            StartMoveArrow();
    }

    IEnumerator Co_WaitAniProgress()
    {
        if (Ani_Progress == null)
            yield break;

        while (Ani_Progress.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
        {
            yield return null;
        }

        var resultDlg = UIFormManager.Instance.GetUIForm<IntotheFireResultDlg>();
        if (resultDlg != null)
        {
            resultDlg.SetAddItemCount(m_bounsPoint);
            resultDlg.SetDifficulty((eCUSTUM_EVENT_DIFFICULTY)m_Difficulty);
            resultDlg.SetEnemyCount(m_AniProgressNum);
            resultDlg.Open();
        }
    }

    public void CheckCurRoundState()
    {
        //라운드가 끝났다..!!
        if (m_curEnemyCount <= 0)
        {
            if (IsInvoking(nameof(UpdateRemainTime)))
                CancelInvoke(nameof(UpdateRemainTime));

            StartCoroutine(Co_WaitAniProgress());
        }
        else
        {
            StartCoroutine(Co_WaitStartMoveArrow());
        }
    }

    public void PlayTween()
    {
        if (Tween_Point == null)
            return;

        Tween_Point.PlayForward();
        Tween_Point.AddOnFinished(() =>
        {
            Tween_Point.ResetToBeginning();
        });


        if (m_CheckPoint == true)
        {
            Point_Effect = FXManager.Instance.AcquireInstance(Recycler.KIND_OBJ.FX_UI_GetPoint_01, FX_Point.gameObject, true, 1.0f, LayerMask.NameToLayer("NGUI_2D_Overlay"));
            if (Point_Effect != null)
                Point_Effect.SetActive(true);

            Label_Point.Ex_SetText(m_bounsPoint.ToString());

            m_CheckPoint = false;
        }
    }
}

