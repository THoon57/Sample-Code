using System;
using System.Collections.Generic;
using UnityEngine;
using ScrollGrid;
using BaseTable;
using NLibCs;
using GameEvent;
using Framework.EventSystem;
using CORE.UI.Control;
using DEFINE;
using System.Collections;
using PROTOCOL;

public class WarDiceDlg : UIForm
{
    #region event dispatch

    [ReceiveGameEventAttribute( typeof(GameEvent.ITEM_INFO_UPDATE))]
    protected override void OnDispatchGameEvent(System.Type eventID, Framework.EventSystem.IEventDispatchParam param)
    {
        if (param is GameEvent.ITEM_INFO_UPDATE itemInfo)
        {
            if(_data == null)
                _data = CustomEventManager.Instance.GetCurrentEvent(GameDefine.eCUSTUM_EVENT_TYPE.WARDICE);

            if(_data != null)
            {
                if(itemInfo.m_itemKinds.Contains(_data.KeyItemKind) == true)
                    RefreshPointUI();
            }
        }
    }
    #endregion

    #region Controls

    [AutoHierarchyBinding]
    private class Group_RewardItem_Binder
    {
        public GameObject Mine              = null;
        public UIButton Button_First_1      = null;
        public UILabel Label_Point          = null;
        public UILabel Label_First_1        = null;

        public UISprite Sprite_RewardGlow   = null;
        public UISprite Sprite_Arrow        = null;
        public UISprite Sprite_First_1      = null;
        public UISprite Sprite_ItmeBG       = null;
        public UISprite Sprite_check        = null;
    }

    UILabel Label_HeadTitle                 = null;
    UILabel Label_Title                     = null;
    UILabel Label_Time                      = null;

    UILabel Label_Point                     = null;
    UISprite Sprite_Point                   = null;

    UIButton Button_Question                = null;

    UILabel Label_RewardTitle               = null;
    UISprite Sprite_Select                  = null;
    UIButton Reward_Item                    = null;
    UISprite Sprite_RewardItem              = null;
    UISprite Sprite_RewardGrade             = null;

    UILabel Label_Select                    = null;
    UISprite Sprite_Point_Bottom            = null;
    UISprite Sprite_Line                    = null;
    UILabel Label_PointNum                  = null;

    UITexture Texture_illust                = null;            // 배경 이미지 : BGImage

    UIButton Button_Return                  = null;
    UIButton Button_Roll                    = null;
    UILabel Label_Start                     = null;

    GameObject Group_RoulettePlate          = null;

    //Group_RewardItem_Binder First_1         = null; // 이걸 clone 해서 사용...
    //GameObject Reward_List                  = null; // clone 한걸 여기에 붙인다.

    UILabel Label_My_Point                  = null;

    List<UILabel> Arr_Label_Num             = null;

    GameObject FX_Getreward                 = null;
    GameObject FX_Selectreward              = null;
    GameObject Sprite_Mark                  = null;

    GameObject Thumb_1                      = null;

    UILabel Label_RewardNum                 = null;
    UILabel Label_RewardVol                 = null;

    UIButton Button_Rate                    = null;
    UILabel Label_Rate                      = null;
    UILabel Label_DiceResult = null;
    UILabel Label_Round = null;
    UIButton Button_BackCollider = null;
    TweenScale Tween_Dice = null;
    UIButton Button_KeyItem = null;

    Coroutine CoMoveSoldier = null;
    Coroutine CoClearRound = null;
    Animator Ani_Player = null;
    Animator Ani_Dice = null;
    Animator Ani_DiceResult = null;
    List<GameObject> Ani_Blocks = null;
    List<Animator> Ani_Magnifications = null;
    List<UILabel> Label_Magnifications = null;

    AnimEventReceiver Receiver_Dice = null;
    AnimEventReceiver Receiver_DiceResult = null;

    GameObject Soldier = null;
    GameDefine.eWARDICE_CITY_TYPE SelectCityType = GameDefine.eWARDICE_CITY_TYPE.NONE;

    UICom_TweenController BigCityInfo_Paris = null;
    UICom_TweenController BigCityInfo_London = null;
    UICom_TweenController BigCityInfo_Berlin = null;
    UICom_TweenController BigCityInfo_Moskva = null;
    UICom_TweenController BigCityInfo_Roma = null;

    Camera WarDiceCamera = null;
    //3D 발판에 붙일.. 더미들
    List<GameObject> List_DM_Items = null;
    List<GameObject> List_FX_Items = new List<GameObject>();

    [AutoHierarchyBinding]
    private class WarDiceCityItem
    {
        public GameObject _GameObject = null;

        public UILabel Label_Title1 = null;
        public UILabel Label_Title2 = null;
        public UILabel Label_Summary = null;
        public UILabel Label_Item_Count = null;
        public UILabel Label_Item_Vol = null;
        public UISprite Sprite_ItemIcon = null;
    }

    List<WarDiceCityItem> List_CityItems = null;

    #endregion

    private List<GameObject> m_listReward = new List<GameObject>();

    protected CustomEventData _data = null;
    public CustomEventData EventData { get { return _data; } }

    public float m_TotalTime = 3.0f; //6.0f;
    public int m_DefaultRotate = 2;  //5;


    private int m_iCurNumber    = 0;

    private float m_SliderValue_Target = 0.0f;

    private bool m_bIsCoRunning = false;

    public delegate void VoidDelegate();
    private VoidDelegate m_Callback = null;

    private float m_defaultPosX_SV      = 0.0f;

    private int m_NowBlock = 0;
    private int m_CurDiceResult = 0;

    private EVENT.DummyEventDirection m_dummyEvent = null;

    private int CurCameraLayer
    {
        get
        {
            int ngui3dOverlay = 1 << LayerMask.NameToLayer("NGUI_3D_Overlay");
            int ngui3d = 1 << LayerMask.NameToLayer("NGUI_3D");

            return ngui3d | ngui3dOverlay;
        }
    }

    #region override func
    public override void BindUIControls()
    {
        base.BindUIControls();
        this.ApplyBoundGameObjectGroup();
    }

    public override void BindUIEvents()
    {
        base.BindUIEvents();

        BindEvent(Button_BackCollider.BindButtonClick() , Onclick_Plate);
        BindEvent(Button_Return.BindButtonReturn()      , OnClick_Return);
        BindEvent(Reward_Item.BindButtonClick()         , OnClick_MainReward);
        BindEvent(Button_Roll                           , OnClick_Confirm);
        BindEvent(Button_Question.BindButtonClick()     , OnClick_Info);
        BindEvent(Button_Rate.BindButtonClick()         , OnClick_Rate);
        BindEvent(Button_KeyItem                        , OnClick_DiCeItem);
    }

    public override void Init()
    {
        base.Init();
        UIType = eUIType.UI_POPUP;

        m_iCurNumber        = 0;


        Receiver_Dice.onAnimEvent += OnDiceAnimEvent;
        Receiver_DiceResult.onAnimEvent += OnDicResultAnimEvent;

        //처음에 다 꺼둔다..!

        BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);
        BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);
        BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);
        BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);
        BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);

    }

    public override void Open()
    {
        base.Open();

        CustomEventData curEvent = CustomEventManager.Instance.GetCurrentEvent(GameDefine.eCUSTUM_EVENT_TYPE.WARDICE);
        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetCurrentRoundData(GameDefine.eCUSTUM_EVENT_TYPE.WARDICE);
        if (null == curEvent || null == curRoundData)
        {
            Close();
            return;
        }

        _data = curEvent;

        EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(m_dummyEvent = new EVENT.DummyEventDirection());

        if (IsInvoking(nameof(UpdateRemainTime)))
            CancelInvoke(nameof(UpdateRemainTime));
        InvokeRepeating(nameof(UpdateRemainTime), 0.0f, 1.0f);

        SetUI();
        RefreshMainReward();
        DownloadImage(curEvent, curRoundData);

        if (CustomEventManager.Instance.GetNew(_data))
            CustomEventManager.Instance.SetNew(_data);


        PublicSoundMethod.PlayUISound("sfx_ui_wardice_enter");

    }
    public override void Close()
    {
        base.Close();

        if (IsInvoking(nameof(UpdateRemainTime)))
            CancelInvoke(nameof(UpdateRemainTime));

        AudioManager.Instance.StopSFX("sfx_ui_roulette");

        foreach (var fx in List_FX_Items)
        {
            if (fx != null)
                fx.ReleaseRecycleInstance();
        }
        List_FX_Items.Clear();

        Receiver_Dice.onAnimEvent -= OnDiceAnimEvent;
        Receiver_DiceResult.onAnimEvent -= OnDicResultAnimEvent;


        if(CoMoveSoldier != null)
        {
            StopCoroutine(CoMoveSoldier);
            CoMoveSoldier = null;
        }

        if (CoClearRound != null)
        {
            StopCoroutine(CoClearRound);
            CoClearRound = null;
        }

        if (m_dummyEvent != null)
        {
            m_dummyEvent.End();
            m_dummyEvent = null;
        }
    }

    private void HideCityInfo()
    {
        switch (SelectCityType)
        {
            //트윈 스케일 역재생
            case GameDefine.eWARDICE_CITY_TYPE.Paris: BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
            case GameDefine.eWARDICE_CITY_TYPE.London: BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
            case GameDefine.eWARDICE_CITY_TYPE.Berlin: BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
            case GameDefine.eWARDICE_CITY_TYPE.Moskva: BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
            case GameDefine.eWARDICE_CITY_TYPE.Roma: BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
        }

        SelectCityType = GameDefine.eWARDICE_CITY_TYPE.NONE;
    }

    private void OnDicResultAnimEvent(AnimationEvent animEvent)
    {
        if (animEvent == null)
            return;

        if (_data == null)
            return;

        if (m_CurDiceResult == 0)
            return;

        if (animEvent.stringParameter == "SoliderMoveStart")
        {
            //플레이어 이동 시작
            CoMoveSoldier = StartCoroutine(Co_StartSoldierMove());
        }
    }

    private void OnDiceAnimEvent(AnimationEvent animEvent)
    {
        if (animEvent == null)
            return;

        if (_data == null)
            return;

        if (m_CurDiceResult == 0)
            return;

        if(animEvent.stringParameter == "StartDiceResult")
        {
            //주사위 결과 연출 시작.
            PlayDiceResultAni();
        }
    }

    public override void Refresh()
    {
        base.Refresh();

        RefreshPointUI();
        RefreshRewardUI();
        RefreshMainReward();
        RefreshButton();
        RefreshRoundUI();
        RefreshBackPlate();
        RefreshMagnification();
    }

    #endregion


    private void DownloadImage(CustomEventData eventData, CustomEventRoundData roundData)
    {
        //// 라운드 별로 변경되는 이미지
        //RequestImageLoad(roundData.CustomImage, (wwwdata, error) =>
        //{
        //    if (wwwdata != null && _data != null && _data.EventID == roundData.EventID && _data.Round == roundData.Round)
        //    {
        //        bool success = true;
        //        success &= ShowImage(Texture_WebBanner, wwwdata.MainTex, false);
        //        success &= ShowImage(Texture_WebBanner, wwwdata.AlphaTex, true);
        //        if (success)
        //            Texture_WebBanner.SetActive(true);
        //    }
        //    else
        //    {
        //        NDebug.LogError(error);
        //    }
        //});

        // 배경 일러스트
        RequestImageLoad(eventData.BGImage, (wwwdata, error) =>
        {
            if (wwwdata != null && _data != null)
            {
                bool success = true;
                success &= ShowImage(Texture_illust, wwwdata.MainTex, false);
                success &= ShowImage(Texture_illust, wwwdata.AlphaTex, true);
                if (success)
                    Texture_illust.SetActive(true);
            }
            else
            {
                NDebug.LogError(error);
            }
        });

        //// 빙고 타일 일러스트
        //RequestImageLoad(eventData.NumberBGImage, (wwwdata, error) =>
        //{
        //    if (wwwdata != null && _data != null)
        //    {
        //        ShowImage(Texture_KeyBlackOut, wwwdata.MainTex, false);
        //        ShowImage(Texture_KeyBlackOut, wwwdata.AlphaTex, true);
        //    }
        //    else
        //    {
        //        NDebug.LogError(error);
        //    }
        //});
    }

    private const string FORMAT_IMAGE_DOWNLOAD = "{0}/event_image/{1}/";
    private const string IMAGE_FOLDER_NAME = "total";

    private bool RequestImageLoad(string imgName, Action<HttpImage, string> callBack, bool imageLocalize = false)
    {
        if (_data == null)
            return false;

        string folderName = imageLocalize ? $"{IMAGE_FOLDER_NAME}/{Language.Instance.GetCode_BaseLanguage()}" : IMAGE_FOLDER_NAME;
        string url = string.Format(FORMAT_IMAGE_DOWNLOAD, WOTSettings.CDNURL, folderName);
        HttpImageDownloadManager.Instance.ShopImageDownload(url, imgName, callBack, folderName);
        return true;
    }

    private bool ShowImage(List<UITexture> uiTextureComponents, Texture downloadTex, bool isAlpha)
    {
        bool ret = true;
        foreach (var uiTextureComponent in uiTextureComponents)
            ret &= ShowImage(uiTextureComponent, downloadTex, isAlpha);
        return ret;
    }

    private bool ShowImage(UITexture uiTextureComponent, Texture downloadTex, bool isAlpha)
    {
        if (null == downloadTex)
        {
            NDebug.LogError("Imageinfo is null");
            return false;
        }

        if (null == uiTextureComponent)
        {
            NDebug.LogError("uiTextureComponent - missing UITexture");
            return false;
        }

        uiTextureComponent.enabled = true;

        if (false == isAlpha)
        {
            uiTextureComponent.mainTexture = downloadTex;
            if (uiTextureComponent.material != null)
                uiTextureComponent.material.mainTexture = downloadTex;
        }

        if (null != uiTextureComponent.material)
        {
            var material = new Material(uiTextureComponent.material);
            if (material != null)
            {
                uiTextureComponent.material = material;

                if (isAlpha) uiTextureComponent.material.SetTexture("_MainTransTex", downloadTex);
                else         uiTextureComponent.material.SetTexture("_MainTex", downloadTex);
            }
        }

        return true;
    }

    public void ThrowDice(int diceResult, VoidDelegate callback)
    {
        m_CurDiceResult = diceResult;
        m_Callback = callback;
        Label_DiceResult.Ex_SetText(m_CurDiceResult.ToString());

        if (Ani_Dice != null)
        {
            Ani_Dice.Ex_SetActive(true);
            Ani_Dice.Rebind();
            Ani_Dice.Play(string.Format("FX_UI_Wardice_Dices_{0:D2}", m_CurDiceResult));
        }
    }
    public void PlayDiceResultAni()
    {
        if(Ani_DiceResult != null)
        {
            Ani_DiceResult.Ex_SetActive(true);
            Ani_DiceResult.Rebind();
            Ani_DiceResult.Play(string.Format("FX_UI_Dice_Result_{0:D2}", m_CurDiceResult));
        }
    }

    public void StartRoundClearAni()
    {
        if (CustomEventManager.Instance.IsAllClear(_data.EventID) == false)
        {
            if (CoClearRound != null)
                StopCoroutine(CoClearRound);

            CoClearRound = StartCoroutine(Co_ClearRound());
        }
    }

    IEnumerator Co_ClearRound()
    {
        if (_data == null)
            yield break;

        if (List_FX_Items == null || List_FX_Items.Count <= 0)
            yield break;

        if (List_DM_Items == null || List_DM_Items.Count <= 0)
            yield break;

        if (Ani_Magnifications == null || Ani_Magnifications.Count <= 0)
            yield break;

        Animator ani_LastItem = null;

        for (int i = 0; i < List_FX_Items.Count; ++i)
        {
            if (List_FX_Items[i].TryGetComponent<GameObjectBinder>(out var binder))
            {

                if (i == List_FX_Items.Count - 1)
                {
                    ani_LastItem = binder.FindComponent<Animator>("FX_UI_Diceitem_01");
                }
                else
                {
                    var ani_DiceItem = binder.FindComponent<Animator>("FX_UI_Diceitem_01");
                    if (ani_DiceItem != null)
                    {
                        ani_DiceItem.Rebind();
                        ani_DiceItem.Play("FX_UI_Diceitem_01_Reset");
                    }
                }
            }
        }

        for(int i = 0;  i < Ani_Magnifications.Count; ++i)
        {
            Ani_Magnifications[i].Rebind();
            Ani_Magnifications[i].Play("City_Bonus");
        }

        //마지막 발판까지 연출 재생이 완료 되면 다시 움직인다.
        ani_LastItem.Rebind();
        ani_LastItem.Play("FX_UI_Diceitem_01_Reset");

        while (ani_LastItem.GetCurrentAnimatorStateInfo(0).normalizedTime < 1f)
            yield return null;

        if (CoMoveSoldier != null)
            StopCoroutine(CoMoveSoldier);

        CoMoveSoldier = StartCoroutine(Co_StartSoldierMove());
    }

    IEnumerator Co_StartSoldierMove()
    {
        if (_data == null)
            yield break;

        if(List_DM_Items == null || List_DM_Items.Count <=0)
            yield break;

        if (Ani_Player == null || Soldier == null)
            yield break;

        if (List_FX_Items == null || List_FX_Items.Count <= 0)
            yield break;

        var blockIndex = 0;
        var realMoveCount = 0;
        for (int i = 0; i < m_CurDiceResult; ++i)
        {
            blockIndex = m_NowBlock + 1;

            if (blockIndex >= List_DM_Items.Count)
                blockIndex -= List_DM_Items.Count;

            if(blockIndex >= List_DM_Items.Count)
                yield break;

            var nowPos = Soldier.transform.position;
            var nextPos = List_DM_Items[blockIndex].transform.position;
            nextPos.y = 0f;

            var aniLenght = Ani_Player.GetCurrentAnimatorStateInfo(0).length;
            float moveSpeed = 0f;

            Ani_Player.Rebind();
            Ani_Player.Play("FX_UI_Wardicce_Player_Jump");

            while (moveSpeed < aniLenght)
            {
                moveSpeed += Time.deltaTime;
                Soldier.transform.position = Vector3.Lerp(nowPos, nextPos, moveSpeed / aniLenght);

                yield return null;
            }
            ++realMoveCount;
            m_NowBlock = blockIndex;

            //도착하면 블럭 깜빡!
            if (blockIndex >= Ani_Blocks.Count)
                yield break;

            var ani_Block = Ani_Blocks[blockIndex].GetComponent<Animator>();
            var cityController = Ani_Blocks[blockIndex].GetComponent<WarDiceCityController>();
            if (ani_Block == null)
                yield break;

            ani_Block.Rebind();

            //보상 받을 발판
            if (realMoveCount == m_CurDiceResult)
            {
                Button_Roll.enabled = true;

                if (blockIndex >= List_FX_Items.Count)
                    yield break;

                if (cityController != null)
                    ani_Block.Play(string.Format("Wardice_block_Reward_{0}", cityController.CityType.ToString()));
                else
                    ani_Block.Play("Wardice_block_Jump");

                //아이템 사라지는 연출
                if (List_FX_Items[blockIndex].TryGetComponent<GameObjectBinder>(out var binder))
                {
                    var ani_DiceItem = binder.FindComponent<Animator>("FX_UI_Diceitem_01");
                    if(ani_DiceItem != null)
                    {
                        ani_DiceItem.Rebind();
                        ani_DiceItem.Play("FX_UI_Diceitem_01_Get");
                    }
                }
                RefreshPointUI();
            }
            else
            {

                if (cityController != null)
                    ani_Block.Play(string.Format("Wardice_block_Jump_{0}", cityController.CityType.ToString()));
                else
                    ani_Block.Play("Wardice_block_Jump");
            }

            if (m_NowBlock == 0 && m_Callback != null)
            {
                m_CurDiceResult -= realMoveCount;
                m_Callback();
                yield break;
            }
        }

    }
    public void SetUI()
    {
        if (null == _data)
            return;

        UIBinderExtensionMethods.SetLabel(ref Label_HeadTitle, CustomEventManager.Instance.GetText(_data.TitleKey));
        UIBinderExtensionMethods.SetLabel(ref Label_Title, CustomEventManager.Instance.GetText(_data.TitleKey));
        UIBinderExtensionMethods.SetLabel(ref Label_Start, NTextManager.Instance.GetText("UI_WARDICE_ROUND_03"));
        UIBinderExtensionMethods.SetLabel(ref Label_Rate, NTextManager.Instance.GetText("UI_GOLDSHOP_MESSAGE_BUY_RATE_INFO"));

        // Set Roulette Number
        var sortedSetProbability = CustomEventManager.Instance.GetProbabilityData(_data.EventID);
        if (null == sortedSetProbability)
            return;

        RefreshPointUI();
        RefreshRewardUI();
        RefreshButton();
        RefreshRoundUI();
        SetBackPlate();
        SetSolider();
        RefreshMagnification();
        RefreshCityUI();
    }
    private void RefreshCityUI()
    {
        WarDiceCityController controller = null;

        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, _data.Round);
        if (curRoundData == null || curRoundData.Rewards == null || curRoundData.Rewards.Count <= 0)
            return;

        var valueData = CustomEventManager.Instance.GetValueData(_data.EventID, GameDefine.eCUSTUM_EVENT_DIFFICULTY.NONE);
        if (valueData == null)
            return;

        var cityitemKind = 0;
        Int64 cityItemCount = 0;
        var cityUpperStringName = string.Empty;
        for (int i = 0; i < List_CityItems.Count; ++i)
        {
            controller = List_CityItems[i]._GameObject.GetComponent<WarDiceCityController>();
            if (controller == null)
                continue;

            cityUpperStringName = controller.CityType.ToString().ToUpper();

            List_CityItems[i].Label_Title1.Ex_SetText(NTextManager.Instance.GetText("UI_WARDICE_CITY_CLASS_TITLE"));
            List_CityItems[i].Label_Title2.Ex_SetText(NTextManager.Instance.GetText(string.Format("UI_WARDICE_CITY_TITLE_{0}", cityUpperStringName)));
            List_CityItems[i].Label_Summary.Ex_SetText(NTextManager.Instance.GetText(string.Format("UI_WARDICE_CITY_DESC_{0}", cityUpperStringName)));

            switch(controller.CityType)
            {
                case GameDefine.eWARDICE_CITY_TYPE.Paris:
                    {
                        if (valueData.Value1 >= curRoundData.Rewards.Count)
                            continue;

                        cityitemKind = curRoundData.Rewards[valueData.Value1].Rewards[0].kind;
                        cityItemCount = curRoundData.Rewards[valueData.Value1].Rewards[0].num;
                    }
                    break;
                case GameDefine.eWARDICE_CITY_TYPE.London:
                    {
                        if (valueData.Value2 >= curRoundData.Rewards.Count)
                            continue;

                        cityitemKind = curRoundData.Rewards[valueData.Value2].Rewards[0].kind;
                        cityItemCount = curRoundData.Rewards[valueData.Value2].Rewards[0].num;
                    }
                    break;
                case GameDefine.eWARDICE_CITY_TYPE.Berlin:
                    {
                        if (valueData.Value3 >= curRoundData.Rewards.Count)
                            continue;

                        cityitemKind = curRoundData.Rewards[valueData.Value3].Rewards[0].kind;
                        cityItemCount = curRoundData.Rewards[valueData.Value3].Rewards[0].num;
                    }
                    break;
                case GameDefine.eWARDICE_CITY_TYPE.Moskva:
                    {
                        if (valueData.Value4 >= curRoundData.Rewards.Count)
                            continue;

                        cityitemKind = curRoundData.Rewards[valueData.Value4].Rewards[0].kind;
                        cityItemCount = curRoundData.Rewards[valueData.Value4].Rewards[0].num;
                    }
                    break;
                case GameDefine.eWARDICE_CITY_TYPE.Roma:
                    {
                        if (valueData.Value5 >= curRoundData.Rewards.Count)
                            continue;

                        cityitemKind = curRoundData.Rewards[valueData.Value5].Rewards[0].kind;
                        cityItemCount = curRoundData.Rewards[valueData.Value5].Rewards[0].num;
                    }
                    break;
            }

            var itemInfo = TableItemInfo.Instance.Get(cityitemKind);
            if (itemInfo == null)
                continue;

            List_CityItems[i].Sprite_ItemIcon.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);

            if (PublicUIMethod.GetString_TotalItemCount(cityitemKind, cityItemCount, out string totalString))
            {
                List_CityItems[i].Label_Item_Count.SetActive(false);
                List_CityItems[i].Label_Item_Vol.SetActive(true);
                List_CityItems[i].Label_Item_Vol.SetLabel(totalString);
            }
            else
            {
                List_CityItems[i].Label_Item_Count.SetActive(true);
                List_CityItems[i].Label_Item_Vol.SetActive(true);
                List_CityItems[i].Label_Item_Count.SetLabel(PublicUIMethod.ThousandSeparateString(cityItemCount));
                List_CityItems[i].Label_Item_Vol.SetLabel(PublicUIMethod.GetString_ItemVolumeText(itemInfo));
            }

        }

    }
    private void RefreshMagnification()
    {
        if (_data == null)
            return;

        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, _data.Round);
        if (curRoundData == null || curRoundData.RoundGoal == null || curRoundData.RoundGoal.Count <= 0)
            return;

        if (Label_Magnifications == null || Label_Magnifications.Count <= 0 || Label_Magnifications.Count != curRoundData.RoundGoal.Count)
            return;

        for (int i = 0; i < Label_Magnifications.Count; ++i)
            Label_Magnifications[i].Ex_SetText(string.Format("x{0}", curRoundData.RoundGoal[i].ToString()));

    }
    private void SetSolider()
    {
        if (_data == null)
            return;

        if (Soldier == null || List_DM_Items == null || List_DM_Items.Count <= 0)
            return;

        if(_data is WarDiceEventData warDiceData)
        {
            var blockIndex = warDiceData.DiceBlock;
            m_NowBlock = blockIndex;

            if (blockIndex >= List_DM_Items.Count)
                return;

            Vector3 soldierPos = Vector3.zero;
            if (CustomEventManager.Instance.IsAllClear(_data.EventID) == true)
                soldierPos = List_DM_Items[0].transform.position;
            else
                soldierPos = List_DM_Items[blockIndex].transform.position;

            soldierPos.y = 0f;
            Soldier.transform.position = soldierPos;

            Ani_Player.Rebind();
            Ani_Player.Play("FX_UI_Wardicce_Player_Idle");
        }
    }
    private void SetBackPlate()
    {
        if (_data == null)
            return;

        foreach (var fx in List_FX_Items)
        {
            if (fx != null)
                fx.ReleaseRecycleInstance();
        }
        List_FX_Items.Clear();

        if (CustomEventManager.Instance.IsAllClear(_data.EventID) == true)
            return;

        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, _data.Round);
        if (curRoundData == null || curRoundData.Rewards == null || curRoundData.Rewards.Count <= 0)
            return;

        if (List_DM_Items == null || List_DM_Items.Count <= 0)
            return;

        //라운드 보상 + 발판 보상 27개 
        if (List_DM_Items.Count != curRoundData.Rewards.Count)
            return;

        for (int i = 0; i < List_DM_Items.Count; ++i)
        {
            if (curRoundData.Rewards[i].Rewards != null && curRoundData.Rewards[i].Rewards.Count > 0)
            {
                var itemKind = curRoundData.Rewards[i].Rewards[0].kind;
                var itemCount = curRoundData.Rewards[i].Rewards[0].num;

                var itemInfo = TableItemInfo.Instance.Get(itemKind);
                if (itemInfo == null)
                    continue;

                var fx_DiceItem = FXManager.Instance.AcquireInstance(Recycler.KIND_OBJ.FX_UI_Diceitem_01, List_DM_Items[i], false, 1.0f, LayerMask.NameToLayer("NGUI_3D_Overlay"));
                if (fx_DiceItem != null && fx_DiceItem.TryGetComponent<GameObjectBinder>(out var binder))
                {
                    fx_DiceItem.transform.InitLocal();

                    if (curRoundData.Rewards[i].bRewarded == true)
                        List_DM_Items[i].Ex_SetActive(false);
                    else
                        List_DM_Items[i].Ex_SetActive(true);

                    List_FX_Items.Add(fx_DiceItem);

                    //첫번째꺼는 더미
                    if (i == 0)
                    {
                        fx_DiceItem.Ex_SetActive(false);
                        continue;
                    }

                    var uiLabel = binder.FindComponent<UILabel>("Label_Icon");
                    var uiSprite = binder.FindComponent<UISprite>("Button_Icon");
                    var uiShadowSprite = binder.FindComponent<UISprite>("Sprite_Icon_Shadow");
                    if (uiLabel != null && uiSprite != null)
                    {
                        uiSprite.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);
                        uiShadowSprite.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);

                        PublicUIMethod.GetString_TotalItemCount(itemKind, itemCount, out string totalString);
                        uiLabel.SetLabel(totalString);
                    }
                }
            }
        }
    }
    private void RefreshBackPlate()
    {
        if (_data == null)
            return;

        foreach (var fx in List_FX_Items)
        {
            if (fx != null)
                fx.ReleaseRecycleInstance();
        }
        List_FX_Items.Clear();

        if (CustomEventManager.Instance.IsAllClear(_data.EventID) == true)
            return;

        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, _data.Round);
        if (curRoundData == null || curRoundData.Rewards == null || curRoundData.Rewards.Count <= 0)
            return;

        if (List_DM_Items == null || List_DM_Items.Count <= 0)
            return;

        //라운드 보상 + 발판 보상 27개 
        if (List_DM_Items.Count != curRoundData.Rewards.Count)
            return;

        for(int i = 0; i < List_DM_Items.Count; ++i)
        {
            if (curRoundData.Rewards[i].Rewards != null && curRoundData.Rewards[i].Rewards.Count > 0)
            {
                var itemKind = curRoundData.Rewards[i].Rewards[0].kind;
                var itemCount = curRoundData.Rewards[i].Rewards[0].num;

                var itemInfo = TableItemInfo.Instance.Get(itemKind);
                if (itemInfo == null)
                    continue;

                var fx_DiceItem = FXManager.Instance.AcquireInstance(Recycler.KIND_OBJ.FX_UI_Diceitem_01, List_DM_Items[i], false, 1.0f, LayerMask.NameToLayer("NGUI_3D_Overlay"));
                if (fx_DiceItem != null && fx_DiceItem.TryGetComponent<GameObjectBinder>(out var binder))
                {
                    fx_DiceItem.transform.InitLocal();
                    List_DM_Items[i].Ex_SetActive(true);
                    List_FX_Items.Add(fx_DiceItem);

                    //첫번째꺼는 더미
                    if(i == 0 )
                    {
                        fx_DiceItem.Ex_SetActive(false);
                        continue;
                    }

                    var uiLabel = binder.FindComponent<UILabel>("Label_Icon");
                    var uiSprite = binder.FindComponent<UISprite>("Button_Icon");
                    var uiShadowSprite = binder.FindComponent<UISprite>("Sprite_Icon_Shadow");
                    if (uiLabel != null && uiSprite != null)
                    {
                        uiSprite.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);
                        uiShadowSprite.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);

                        PublicUIMethod.GetString_TotalItemCount(itemKind, itemCount, out string totalString);
                        uiLabel.SetLabel(totalString);
                    }
                }
            }
        }

    }

    private void RefreshPointUI()
    {
        if (_data == null)
            return;

        UIBinderExtensionMethods.SetLabel(ref Label_Round, string.Format(NTextManager.Instance.GetText("UI_WARDICE_ROUND_01"), _data.Round));

        var iInvenItemNum = Inventory.Instance.GetItemNum(_data.KeyItemKind);
        var iNeedItemNum = _data.KeyItemNum;
        UIBinderExtensionMethods.SetLabel(ref Label_Point, PublicUIMethod.ThousandSeparateString(iInvenItemNum));

        var keyItemInfo = TableItemInfo.Instance.Get(_data.KeyItemKind);
        if (null == keyItemInfo)
            return;

        UIBinderExtensionMethods.SetSprite(ref Sprite_Point, keyItemInfo.atlasName, keyItemInfo.itemIcon);
        UIBinderExtensionMethods.SetSprite(ref Sprite_Point_Bottom, keyItemInfo.atlasName, keyItemInfo.itemIcon);
        UIBinderExtensionMethods.SetLabel(ref Label_PointNum,
            string.Format("{0}/{1}",
            PublicUIMethod.ThousandSeparateString(iInvenItemNum),
            PublicUIMethod.ThousandSeparateString(iNeedItemNum)));

    }

    const float PADDING = 150.0f;
    const float DEFAULT_START_POS = PADDING;
    private float CalcSliderValue(int iRound)
    {
        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, iRound);
        if (curRoundData == null)
            return 0;

        var width = PADDING * m_listReward.Count;

        int curScore = CustomEventManager.Instance.GetEventScore(_data.EventID);
        int maxScore = curRoundData.RoundGoal[curRoundData.RoundGoal.Count - 1];

        var startScore = 0;
        if (curRoundData.Round > 1)
        {
            CustomEventRoundData beforeRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, curRoundData.Round - 1);
            if (beforeRoundData == null)
                return 0;

            if (beforeRoundData.RoundGoal.Count <= 0)
                return 0;

            startScore = beforeRoundData.RoundGoal[beforeRoundData.RoundGoal.Count - 1];
        }

        var sliderBlockWidth = PADDING / width;
        var sliderValue = 0.0f;
        for (int i = 0; i < curRoundData.RoundGoal.Count; ++i)
        {
            if (maxScore == 0)
                break;

            var needPoint = curRoundData.RoundGoal[i];
            var beforeNeedPoint = (i == 0) ? startScore : curRoundData.RoundGoal[i - 1];
            var needPointValue = needPoint - beforeNeedPoint;  // 직전 보상 ~ 다음 보상 사이 값
            var myPointValue = Math.Max(curScore - beforeNeedPoint, 0);   // 직전 보상부터 쌓인 내 포인트

            if (curScore >= needPoint)
                sliderValue += sliderBlockWidth;
            else
                sliderValue += sliderBlockWidth * (myPointValue / (float)needPointValue);
        }

        m_SliderValue_Target = sliderValue;

        return width;
    }

    private void RefreshRoundUI()
    {
        if (_data == null)
            return;

        UIBinderExtensionMethods.SetLabel(ref Label_RewardTitle, string.Format(NTextManager.Instance.GetText("UI_EVENT_REWARD_ROUND"), _data.Round));
    }
    private void RefreshRewardUI()
    {
        CustomEventRoundData curRoundData = CustomEventManager.Instance.GetRoundData(_data.EventID, _data.Round);
        if (curRoundData == null)
            return;


        //부루마블에서는
        //RewardList 가 발판들이 된다(아이템을 띄울)
        //First_1 이 아이템에 대한 정보들.

        //for (int i = 0; i < curRoundData.RoundGoal.Count; ++i)
        //{
        //    if (m_listReward.Count > i)
        //        continue;

        //    var obj = GameObject.Instantiate(First_1.Mine, Reward_List.transform);
        //    if (obj == null)
        //        continue;

        //    obj.SetActive(false);
        //    m_listReward.Add(obj);
        //}

        //First_1.Mine.SetActive(false);

        int curScore = CustomEventManager.Instance.GetEventScore(_data.EventID);
        Label_My_Point.SetLabel(PublicUIMethod.ThousandSeparateString(curScore));

        if (curRoundData.RoundGoal.Count > 0)
            Label_Select.SetLabel(NTextManager.Instance.GetText("UI_WARDICE_ROUND_02"));

        float width = CalcSliderValue(_data.Round);


        // 보상 아이템
        for (int i = 0; i < curRoundData.RoundGoal.Count; ++i)
        {
            int iRewardIndex = i + 1;

            if (false == curRoundData.Rewards.ContainsKey(iRewardIndex))
                continue;

            var reward = curRoundData.Rewards[iRewardIndex];
            if (reward == null || reward.Rewards.Count <= 0)
                continue;
            var rewardItem = reward.Rewards[0];
            if (null == rewardItem)
                continue;

            int itemKind = rewardItem.kind;
            long itemCount = rewardItem.num;

            var itemInfo = TableItemInfo.Instance.Get(itemKind);
            if (null == itemInfo)
                continue;

            if (m_listReward.Count <= i)
                continue;

            var obj = m_listReward[i];
            var binder = obj.GetComponent<GameObjectBinder>();
            var itemSprite = binder.FindComponentInChildren<UISprite>("Sprite_First_1");
            itemSprite.SetSprite(itemInfo.itemIcon, itemInfo.atlasName);

            var itemCountLabel = binder.FindComponentInChildren<UILabel>("Label_First_1");
            PublicUIMethod.GetString_TotalItemCount(itemKind, itemCount, out string totalString);
            itemCountLabel.SetLabel(totalString);

            if (curRoundData.RoundGoal.Count <= i)
                continue;
            var needPoint = curRoundData.RoundGoal[i];

            var itemNeedPoint = binder.FindComponentInChildren<UILabel>("Label_Point");
            itemNeedPoint.SetLabel(needPoint.ToString());

            var itemCheck = binder.FindComponentInChildren<UISprite>("Sprite_check");
            itemCheck.SetActive(reward.bRewarded);

            var itemButton = binder.FindComponentInChildren<UIButton>("Button_First_1");
            itemButton.SetActive(true);

            itemSprite.grayscale = reward.bRewarded;

            int iClickIndex = i + 1;
            BindEvent(itemButton.BindButtonClick(), () => { OnClick_LineReward(iClickIndex); });


            //obj.transform.localPosition = Vector3.right * (startPos + DEFAULT_START_POS + (i * PADDING));
            obj.SetActive(true);
        }
    }

    private void RefreshButton()
    {
        bool bSelectedReward    = (false == CustomEventManager.Instance.CanSelectMainReward(GameDefine.eCUSTUM_EVENT_TYPE.WARDICE));
        bool bCanRoll           = CustomEventManager.Instance.CanPlayEvent(GameDefine.eCUSTUM_EVENT_TYPE.WARDICE);
        Button_Roll.SetEnable(bSelectedReward && bCanRoll);
    }
    public void RefreshMainReward()
    {
        NrItemDataInfo itemInfo = null;

        bool bFailed = false;
        var mainReward = CustomEventManager.Instance.Get_CustomMainReward(_data.EventID);
        if (null == mainReward || mainReward.kind <= 0)
            bFailed = true;
        else
            itemInfo = TableItemInfo.Instance.Get(mainReward.kind);

        if (null == itemInfo)
            bFailed = true;
        
        if (bFailed)
        {
            Sprite_Select.SetActive(true);
            Sprite_RewardItem.SetActive(false);
            Sprite_RewardGrade.SetActive(false);
            Label_RewardVol.SetActive(false);
            Label_RewardNum.SetActive(false);
            FX_Getreward.SetActive(false);
            FX_Selectreward.SetActive(true);
            return;
        }

        Sprite_Select.SetActive(false);
        Sprite_RewardItem.SetActive(true);
        Sprite_RewardGrade.SetActive(true);
        Label_RewardVol.SetActive(true);
        Label_RewardNum.SetActive(true);
        FX_Getreward.SetActive(true);
        FX_Selectreward.SetActive(false);

        if (null != Reward_Item)
            Reward_Item.normalSprite = itemInfo.itemIcon;

        UIBinderExtensionMethods.SetSprite(ref Sprite_RewardItem, itemInfo.atlasName, itemInfo.itemIcon);

        if (PublicUIMethod.GetString_TotalItemCount(mainReward.kind, mainReward.num, out string totalString))
        {
            Label_RewardNum.SetActive(false);
            Label_RewardVol.SetActive(true);
            Label_RewardVol.SetLabel(totalString);
            PublicUIMethod.UpdateItemGrade(mainReward.kind, Sprite_RewardGrade, null, null);
        }
        // 아이템 수량을 합쳐서 표기할 수 없다면...
        else
        {
            Label_RewardNum.SetActive(true);
            Label_RewardVol.SetActive(true);
            Label_RewardNum.SetLabel(PublicUIMethod.ThousandSeparateString(mainReward.num));
            PublicUIMethod.UpdateItemGrade(mainReward.kind, Sprite_RewardGrade, Label_RewardVol, null);
        }

        RefreshButton();
    }
    private void UpdateRemainTime()
    {
        if (null != _data)
        {
            long endTime = _data.EndTime;
            long leftSecond = PublicMethod.leftUTCSeconds(endTime);

            UIBinderExtensionMethods.SetLabel(ref Label_Time, PublicMethod.TimeToString(leftSecond));

            if (leftSecond <= 0)
            {
                // 이벤트 종료!
                Close();
            }
        }
    }
    private void OnClick_Info()
    {
        PublicSoundMethod.PlayClickCommon();

        if (null == _data)
            return;

        HideCityInfo();

        var dlg = UIFormManager.Instance.GetUIForm<BaseQuestionDlg>();
        if (null != dlg)
        {
            dlg.OpenAnswer(
                NTextManager.Instance.GetText("UI_GUILD_HELP_TITLE"),
                CustomEventManager.Instance.GetText(_data.TitleKey),
                CustomEventManager.Instance.GetText(_data.HelpKey)
            );
        }
    }
    private void Onclick_Plate()
    {
        if (_data == null)
            return;

        if (WarDiceCamera == null)
            return;

        var touchPos = Vector3.zero;
        if (Input.touchCount == 1)
            touchPos = Input.GetTouch(0).position;
        else
            touchPos = Input.mousePosition;

        var caemraRay = WarDiceCamera.ScreenPointToRay(touchPos);
        WarDiceCityController cityController = null;
        RaycastHit hit;
        GameDefine.eWARDICE_CITY_TYPE hitCityType = GameDefine.eWARDICE_CITY_TYPE.NONE;
        if (Physics.Raycast(caemraRay, out hit, 2000, CurCameraLayer))
        {
            if (hit.collider.gameObject)
            {
                NDebug.Log(hit.collider.gameObject.name);

                cityController = hit.collider.gameObject.GetComponent<WarDiceCityController>();
                if (cityController != null)
                {
                    if (cityController.CityType == GameDefine.eWARDICE_CITY_TYPE.Paris)
                        hitCityType = GameDefine.eWARDICE_CITY_TYPE.Paris;
                    else if (cityController.CityType == GameDefine.eWARDICE_CITY_TYPE.London)
                        hitCityType = GameDefine.eWARDICE_CITY_TYPE.London;
                    else if (cityController.CityType == GameDefine.eWARDICE_CITY_TYPE.Berlin)
                        hitCityType = GameDefine.eWARDICE_CITY_TYPE.Berlin;
                    else if (cityController.CityType == GameDefine.eWARDICE_CITY_TYPE.Moskva)
                        hitCityType = GameDefine.eWARDICE_CITY_TYPE.Moskva;
                    else if (cityController.CityType == GameDefine.eWARDICE_CITY_TYPE.Roma)
                        hitCityType = GameDefine.eWARDICE_CITY_TYPE.Roma;
                }
            }
        }

        //도시를 클릭하지 않았다.
        if(cityController == null)
        {
            //이전에 선택 된 도시가 있다.
            if (SelectCityType != GameDefine.eWARDICE_CITY_TYPE.NONE)
            {
                //트윈 스케일 반대로 재생 해서 닫기
                switch (SelectCityType)
                {
                    //트윈 스케일 역재생
                    case GameDefine.eWARDICE_CITY_TYPE.Paris: BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);   break;
                    case GameDefine.eWARDICE_CITY_TYPE.London: BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Berlin: BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Moskva: BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Roma: BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);     break;
                }
            }
        }
        //도시를 클릭 했다.
        else
        {
            var ani_block = hit.collider.gameObject.GetComponent<Animator>();
            if (ani_block != null)
            {
                ani_block.Rebind();
                ani_block.Play(string.Format("Wardice_block_Jump_{0}", hitCityType.ToString()));
            }

            //내가 원래 클릭했던 도시랑 다르다
            if (SelectCityType != hitCityType)
            {
                switch (SelectCityType)
                {
                    //트윈 스케일 역재생
                    case GameDefine.eWARDICE_CITY_TYPE.Paris: BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);   break;
                    case GameDefine.eWARDICE_CITY_TYPE.London: BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Berlin: BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Moskva: BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Roma: BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.REVERSE);     break;
                }

                //재생
                switch (hitCityType)
                {
                    case GameDefine.eWARDICE_CITY_TYPE.Paris: BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.FORWARD);   break;
                    case GameDefine.eWARDICE_CITY_TYPE.London: BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.FORWARD); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Berlin: BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.FORWARD); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Moskva: BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.FORWARD); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Roma: BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.FORWARD);     break;
                }

                
            }
            else
            {
                switch (SelectCityType)
                {
                    //트윈 스케일 역재생
                    case GameDefine.eWARDICE_CITY_TYPE.Paris: BigCityInfo_Paris.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.London: BigCityInfo_London.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Berlin: BigCityInfo_Berlin.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Moskva: BigCityInfo_Moskva.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                    case GameDefine.eWARDICE_CITY_TYPE.Roma: BigCityInfo_Roma.PlayTweener(UICom_TweenController.PlayDirection.REVERSE); break;
                }

                hitCityType = GameDefine.eWARDICE_CITY_TYPE.NONE;
            }
        }

        SelectCityType = hitCityType;
    }

    private void OnClick_MainReward()
    {
        if (null == _data)
            return;

        HideCityInfo();

        PublicSoundMethod.PlayClickCommon();
        var dlg = UIFormManager.Instance.GetUIForm<SelectRewardPopUp>();
        if (dlg != null)
            dlg.Open(DEFINE.GameDefine.eCUSTUM_EVENT_TYPE.WARDICE);
    }

    private void OnClick_LineReward(int iRewardType)
    {
        PublicSoundMethod.PlayClickCommon();

        if (null == _data)
            return;

        var rewardItem = CustomEventManager.Instance.Get_CustomEventRewardData(_data.EventID, _data.Round, iRewardType);
        if (rewardItem == null)
            return;
        if (rewardItem.Rewards.Count <= 0)
            return;

        var item = rewardItem.Rewards[0];
        if (item == null)
            return;

        PublicUIMethod.OpenItemDetailsDlg(item.kind, item.num);
    }

    private void OnClick_Confirm()
    {
        if (null == _data)
            return;

        var needItem = CustomEventManager.Instance.GetNeedItem(_data.EventID);
        if (null == needItem)
            return;

        if (Tween_Dice == null)
            return;

        HideCityInfo();

        var itemTableInfo = BaseTable.TableItemInfo.Instance.Get(needItem.kind);
        if (null != itemTableInfo)
        {
            bool bEnought = PublicMethod.GetNeedItemEnough(needItem);

            if (CustomEventManager.Instance.IsAllClear(_data.EventID))
            {
                PublicSoundMethod.PlayClickFail();
                // "최종 라운드 달성했습니다."
                PublicUIMethod.SetPopUpAlramMsg(NTextManager.Instance.GetText("UI_EVENT_ROUND_END"));
                
            }
            // 재료 모자른데 클릭했을 경우 토스트 알림
            else if (false == bEnought)
            {
                PublicSoundMethod.PlayClickFail();
                // 필요 아이템 : {0}
                PublicUIMethod.SetPopUpAlramMsg(string.Format(NTextManager.Instance.GetText("UI_BINGO_EVENT_ERROR_NOTICE"),
                   NTextManager.Instance.GetText(itemTableInfo.itemName)));
            }
            // 메인 보상 선택 안했을 경우 토스트 알림
            else if (0 == _data.MainRewardItemKind)
            {
                PublicSoundMethod.PlayClickFail();
                // 	"다음 라운드 보상을 선택해주세요."
                PublicUIMethod.SetPopUpAlramMsg(NTextManager.Instance.GetText("UI_EVENT_NO_REWARD_NOTICE"));
            }
            // 아이템 교환 요청
            else
            {
                if (NetFB.Instance.IsBlockPacketID(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_PLAY_REQ) == true)
                    return;

                Tween_Dice.PlayForward();
                Tween_Dice.AddOnFinished(() =>
                {
                    Tween_Dice.ResetToBeginning();
                });

                PublicSoundMethod.PlayClickCommon();

                FlatBuffers.FlatBufferBuilder fbb = FlatBuffers.NFlatBufferBuilder.FBB;

                System.Func<object> OffsetMethod = () =>
                {
                    int RoundProgress = 0;
                    if (_data is BingoEventData bingoData)
                        RoundProgress = bingoData.m_BingoData.Count;
                    else if (_data is RouletteEventData rouletteData)
                        RoundProgress = rouletteData.Score;
                    else if (_data is TimingGameEventData intoTheFireData)
                        RoundProgress = intoTheFireData.Score;
                    else if (_data is WarDiceEventData warDiceData)
                        RoundProgress = warDiceData.DiceBlock;

                    return PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_PLAY_REQ.CreateGS_CUSTOM_EVENT_PLAY_REQ(fbb, _data.EventID, _data.KeyItemKind, _data.KeyItemNum, RoundProgress);
                };
                FlatBuffers.NFlatBufferBuilder.SendBytes<PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_PLAY_REQ>(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_PLAY_REQ, OffsetMethod);

                Button_Roll.enabled = false;
            }
        }
    }

    private void OnClick_Rate()
    {
        HideCityInfo();
        CustomWebView.Show(SocialManager.Instance.GetWarDiceProbabilityLinkUrl());
    }

    private void OnClick_Return()
    {
        Close();
    }
    private void OnClick_DiCeItem()
    {
        PublicSoundMethod.PlayClickCommon();
        ShopUIManager.Instance.MoveTabOpen(ShopUIManager.SHOP_TAB_KIND.SPECIAL, true);
    }
}
