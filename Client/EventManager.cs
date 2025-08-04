

using System;
using System.Collections.Generic;
using BaseTable;
using DEFINE;
using GameEvent;
using NLibCs;
using UnityEngine;

using static DEFINE.GameDefine;
using static MainTopDlg;



public class CustomEventData
{
    // 이벤트 정의 데이터
    public long EventID = 0;

    public eCUSTUM_EVENT_SCHEDULE_TYPE ScheduleType = eCUSTUM_EVENT_SCHEDULE_TYPE.NONE;
    public long StartTime = 0;
    public long EndTime = 0;

    public eCUSTUM_EVENT_TYPE EventType = eCUSTUM_EVENT_TYPE.NONE;

    public int QuestGroup = 0;

    public int  KeyItemKind = 0;
    public int  KeyItemNum = 0;

    public string BGImage = null;
    public string NumberBGImage = null;

    public string IconKey = null;
    public string IconSprite = null;
    public string IconAtlas = null;

    public string TitleKey = null;
    public string HelpKey = null;

    public MainTopDlg.eBTN_GROUP_TYPE TapInfo = MainTopDlg.eBTN_GROUP_TYPE.NONE;

    // userData
    public int MainRewardItemKind = 0;
    public int Round = 0;
}

public class BingoEventData : CustomEventData
{
    public List<int> m_BingoData = new List<int>();
}

public class RouletteEventData : CustomEventData
{
    public int Score = 0;
}

public class TimingGameEventData : CustomEventData
{
    public int SucessBar = 0;
    public int GreatSucessBar = 0;
    public int Score = 0;
}
public class WarDiceEventData : CustomEventData
{
    public int DiceBlock = 0;
}

public class CustomEventRewardData
{
    // 이벤트 보상 정의 데이터
    public long RewardGroupID   = 0;
    public long EventID         = 0;
    public int  Round           = 0;

    public int RewardType       = 0; // 0 = AllBingo, 1~12 = LineNumber

    public List<Item> Rewards = new List<Item>();

    // User Data
    public bool bRewarded = false;
}

public class CustomEventRoundData
{
    public long EventID = 0;
    public int Round = 0;
    public List<int> RoundGoal = new List<int>();

    // LineNumber, data
    public Dictionary<int, CustomEventRewardData> Rewards = new Dictionary<int, CustomEventRewardData>();

    public string CustomImage = null;

    public void ClearRewarded()
    {
        foreach (var rewardData in Rewards)
            rewardData.Value.bRewarded = false;
    }
}

public class CustomEventProbabilityData : IComparable<CustomEventProbabilityData>
{
    public long EventID = 0;
    public int ProbKey_1 = 0;
    public int ProbKey_2 = 0;

    public int CompareTo(CustomEventProbabilityData other)
    {
        var first = this.EventID.CompareTo(other.EventID);
        if (first != 0)
            return first;

        var second = this.ProbKey_1.CompareTo(other.ProbKey_1);
        if (second != 0)
            return second;

        return this.ProbKey_2.CompareTo(other.ProbKey_2);
    }
}

public class CustomEventQuestData
{
    public Int64 EventID = 0;
    public Int64 QuestID = 0;
    public Int64 CurValue = 0;
    public Int32 ConditionKind = 0;
    public Int64 ConditionValue = 0;
    public Int64 TargetValue = 0;
    public string PointHelpKey = null;
    public eCUSTOM_EVENT_MISSION_STATE MissionState = eCUSTOM_EVENT_MISSION_STATE.NONE;
    public List<Item> RewardItems = new List<Item>();

}
public class CustomEventValueData
{
    public Int64 EventID = 0;
    public Int32 Difficulty = 0;
    public Int32 Value1 = 0;
    public Int32 Value2 = 0;
    public Int32 Value3 = 0;
    public Int32 Value4 = 0;
    public Int32 Value5 = 0;
    public Int32 Value6 = 0;
    public Int32 Value7 = 0;
    public Int32 Value8 = 0;
    public Int32 Value9 = 0;
    public Int32 Value10 = 0;
}

public class CustomEventManager : NrTSingleton<CustomEventManager>
{
    // eventID, data
    private Dictionary<long, CustomEventData> m_dicEventData = new Dictionary<long, CustomEventData>();
    // eventID, (Round, data)
    private Dictionary<long, Dictionary<int, CustomEventRoundData>> m_dicEvent_RoundData = new Dictionary<long, Dictionary<int, CustomEventRoundData>>();
    // eventID, Probability-SortedSet
    private Dictionary<long, SortedSet<CustomEventProbabilityData>> m_dicEvent_ProbabilityData = new Dictionary<long, SortedSet<CustomEventProbabilityData>>();

    //eventID, (QuestID,QuestData)
    private Dictionary<long, Dictionary<Int64, CustomEventQuestData>> m_dicQuestData = new Dictionary<long, Dictionary<long, CustomEventQuestData>>();
    //eventID, (Difficulty,ValueData)
    private Dictionary<long, Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData>> m_dicValueData = new Dictionary<long, Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData>>();

    static DateTime LastUpdateTime_EventText = default;

    // language code, text dictionary
    private Dictionary<short, TextDictionary> m_dicEventText = new Dictionary<short, TextDictionary>();
    private Dictionary<short, DateTime> m_dicLastUpdateEventText = new Dictionary<short, DateTime>(); // 텍스트 최신화 여부

    private HashSet<MainTopDlg.eBTN_GROUP_TYPE> m_setTapInfo = new HashSet<eBTN_GROUP_TYPE>();

    private CustomEventManager()
    {
    }

    public void ClearAllData()
    {
        m_dicEventData.Clear();
        m_dicEvent_RoundData.Clear();
        m_dicEvent_ProbabilityData.Clear();
        m_dicQuestData.Clear();
        m_dicValueData.Clear();
        m_setTapInfo.Clear();
    }

    public CustomEventData GetCustomEventData(long eventID)
    {
        if (m_dicEventData.ContainsKey(eventID))
            return m_dicEventData[eventID];
        else
            return null;
    }

    public SortedSet<CustomEventProbabilityData> GetProbabilityData(long eventID)
    {
        if (m_dicEvent_ProbabilityData.ContainsKey(eventID))
            return m_dicEvent_ProbabilityData[eventID];
        else
            return null;
    }

    public bool IsAllClear(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return false;

        if (m_dicEvent_RoundData.ContainsKey(eventID) == false)
            return false;

        bool isMaxRound = curEvent.Round == m_dicEvent_RoundData[eventID].Count;

        var rewardData_Main = CustomEventManager.Instance.Get_CustomEventRewardData(curEvent.EventID, curEvent.Round, 0);
        bool IsRoundFinish  = (null != rewardData_Main && rewardData_Main.bRewarded);

        return isMaxRound && IsRoundFinish;
    }
    public bool IsMaxRound(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return false;

        return curEvent.Round == m_dicEvent_RoundData[eventID].Count;
    }
    public int GetMaxRound(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return 0;

        if (m_dicEvent_RoundData.ContainsKey(eventID))
        {
            return m_dicEvent_RoundData[eventID].Count;
        }

        return 0;
    }

    public Dictionary<Int64, CustomEventQuestData> Get_MissionData_ByEventID(long eventID)
    {
        if (m_dicQuestData.TryGetValue(eventID, out var findInfo))
            return findInfo;

        return null;
    }
    public CustomEventRewardData Get_CustomEventRewardData(long eventID, int iRound, int iRewardType)
    {
        var curRound = GetRoundData(eventID, iRound);
        if (null == curRound)
            return null;

        if (curRound.Rewards.ContainsKey(iRewardType))
        {
            return curRound.Rewards[iRewardType];
        }

        return null;
    }


    // 선택된 메인 보상 Get
    public Item Get_CustomMainReward(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return null;

        if (false == m_dicEvent_RoundData.ContainsKey(eventID))
            return null;

        if (false == m_dicEvent_RoundData[eventID].ContainsKey(curEvent.Round))
            return null;

        var roundData = m_dicEvent_RoundData[eventID][curEvent.Round];
        if (null == roundData)
            return null;

        if (false == roundData.Rewards.ContainsKey(0))
            return null;

        foreach(var elem in roundData.Rewards[0].Rewards)
        {
            if (elem.kind == curEvent.MainRewardItemKind)
            {
                return new Item(elem.kind, elem.num);
            }
        }

        return null;
    }

    public List<Item> GetMainRewards_ByRound(long eventID, int iRound)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return null;

        List<Item> listRewards = new List<Item>();

        var rewardData = Get_CustomEventRewardData(eventID, iRound, 0);
        if (rewardData == null)
            return null;

        for (int i = 0; i < rewardData.Rewards.Count; ++i)
        {
            if (false == PublicMethod.IsValidItemByServerAge(rewardData.Rewards[i].kind))
                continue;

            listRewards.Add(rewardData.Rewards[i]);
        }

        return listRewards;
    }

    public int GetEventScore(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return 0;

        if (curEvent is RouletteEventData rouletteData)
            return rouletteData.Score;
        else if (curEvent is TimingGameEventData timingGameData)
            return timingGameData.Score;

        return 0;
    }

    public bool IsHaveActiveEvent(eCUSTUM_EVENT_TYPE eventType)
    {
        var nowTime = PublicMethod.GetDueDay_UTC(PublicMethod._serverUTCTimeValue);

        foreach(var data in m_dicEventData.Values)
        {
            if (data.EventType != eventType)
                continue;

            if (nowTime > data.StartTime && nowTime < data.EndTime)
                return true;
        }

        return false;
    }

    // 활성화된 이벤트가 여러개일 경우 우선순위 높은 1개만 Icon을 띄웁니다.
    public CustomEventData GetCurrentEvent(eCUSTUM_EVENT_TYPE eventType)
    {
        var nowTime = PublicMethod.GetDueDay_UTC(PublicMethod._serverUTCTimeValue);

        var dataList = new List<CustomEventData>();

        foreach (var data in m_dicEventData.Values)
        {
            if (data.EventType != eventType)
                continue;

            if (nowTime > data.StartTime && nowTime < data.EndTime)
            {
                dataList.Add(data);
            }
        }

        dataList.Sort((left, right) =>
        {
            if (left.ScheduleType > right.ScheduleType)
                return 1;
            else if (left.ScheduleType < right.ScheduleType)
                return -1;
            else
                return 0;
        });

        if (0 < dataList.Count)
            return dataList[0];

        return null;
    }

    public CustomEventRoundData GetRoundData(long eventID, int iRound)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return null;

        if (false == m_dicEvent_RoundData.ContainsKey(eventID))
            return null;

        if (m_dicEvent_RoundData[eventID].ContainsKey(iRound))
            return m_dicEvent_RoundData[eventID][iRound];

        return null;
    }
    public Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> GetValueData(long eventID)
    {
        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> result = null;

        m_dicValueData.TryGetValue(eventID, out result);

        return result;
    }

    public Dictionary<int, CustomEventRoundData> GetTotalRoundData(long eventID)
    {
        var curEvent = GetCustomEventData(eventID);
        if (null == curEvent)
            return null;

        if (true == m_dicEvent_RoundData.ContainsKey(eventID))
            return m_dicEvent_RoundData[eventID];

        return null;
    }

    public Int32 GetArrowSpeedTime(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {
        Int32 result = 0;

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return result;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return result;

        return valueData.Value5;
    }


    public CustomEventRoundData GetCurrentRoundData(eCUSTUM_EVENT_TYPE eventType)
    {
        var curEvent = GetCurrentEvent(eventType);
        if (null == curEvent)
            return null;

        long eventID = curEvent.EventID;

        if (false == m_dicEvent_RoundData.ContainsKey(eventID))
            return null;

        if (m_dicEvent_RoundData[eventID].ContainsKey(curEvent.Round))
            return m_dicEvent_RoundData[eventID][curEvent.Round];

        return null;
    }

    public bool IsOpenPanel(long eventID, int bingoNumber)
    {
        if (false == m_dicEventData.TryGetValue(eventID, out CustomEventData eventData))
            return false;

        if (eventData is not BingoEventData bingoData)
            return false;

        var curBingoData = bingoData.m_BingoData;
        foreach (var elem in curBingoData)
        {
            if (elem == bingoNumber)
                return true;
        }

        return false;
    }
    public bool IsRewarded(long eventID, int iRound, int iRewardType)
    {
        var rewardData = Get_CustomEventRewardData(eventID, iRound, iRewardType);
        
        if (null != rewardData)
        {
            return rewardData.bRewarded;
        }
        
        return false;
    }
    public Item GetNeedItem(long eventID)
    {
        if (m_dicEventData.ContainsKey(eventID))
        {
            if (null == m_dicEventData[eventID])
                return null;
            
            return new Item(m_dicEventData[eventID].KeyItemKind, m_dicEventData[eventID].KeyItemNum);
        }

        return null;
    }

    public bool GetEventPeriod_byItemKind(int itemKind, out (long, long) period, out eCUSTUM_EVENT_TYPE eventType)
    {
        var nowTime = PublicMethod.GetDueDay_UTC(PublicMethod._serverUTCTimeValue);

        foreach (var pair in m_dicEventData)
        {
            if (nowTime < pair.Value.StartTime || pair.Value.EndTime < nowTime)
                continue;


            if (pair.Value.KeyItemKind != itemKind)
                continue;


            period = (pair.Value.StartTime, pair.Value.EndTime);
            eventType = pair.Value.EventType;
            return true;
        }

        period = (0, 0);
        eventType = eCUSTUM_EVENT_TYPE.NONE;
        return false;
    }

    private const string NOTICE_KEY = "new_custom_event_{0}_{1}";
    
    // New Event 인지 체크
    public bool GetNew(CustomEventData curData)
    {
        string strStartTime = PlayerPrefs.GetString(string.Format(NOTICE_KEY, curData.EventID, User.Instance.UID.ToString()), "0");
        long lStartTime = long.Parse(strStartTime);
        return (lStartTime != curData.StartTime);
    }

    public void SetNew(CustomEventData curData)
    {
        PlayerPrefs.SetString(string.Format(NOTICE_KEY, curData.EventID, User.Instance.UID.ToString()), curData.StartTime.ToString());
    }

    public int TotalRewardableCount(long lCurEventID)
    {
        return 0;
    }

    public bool CanSelectMainReward(eCUSTUM_EVENT_TYPE eventType)
    {
        var curEvent = GetCurrentEvent(eventType);
        if (null == curEvent)
            return false;

        return (curEvent.MainRewardItemKind == 0);
    }

    public bool CanPlayEvent(eCUSTUM_EVENT_TYPE eventType)
    {
        var curEvent = GetCurrentEvent(eventType);
        if (null == curEvent)
            return false;


        if (eventType != eCUSTUM_EVENT_TYPE.TIMINGGAME)
        {
            if (false == PublicMethod.GetNeedItemEnough(new Item(curEvent.KeyItemKind, curEvent.KeyItemNum)))
                return false;
        }

        if (CurRoundFinished(curEvent.EventID))
            return false;

        return true;
    }

    public Int32 CanPlayEventCount(eCUSTUM_EVENT_TYPE eventType)
    {
        Int32 result = 0;

        var curEvent = GetCurrentEvent(eventType);
        if (null == curEvent)
            return result;

        result = (Int32)Inventory.Instance.GetItemNum(curEvent.KeyItemKind);

        if (eventType == eCUSTUM_EVENT_TYPE.TIMINGGAME)
            result /= curEvent.KeyItemNum;

        return result;
    }

    public bool CurRoundFinished(long lCurEventID)
    {
        var curEvent = GetCustomEventData(lCurEventID);
        if (null == curEvent)
            return false;

        var rewardData_Main = CustomEventManager.Instance.Get_CustomEventRewardData(curEvent.EventID, curEvent.Round, 0);
        if (null != rewardData_Main && rewardData_Main.bRewarded)
            return true;

        return false;
    }
    public void AddText(short languageCode, string textKey, string textValue)
    {
        if (m_dicEventText.ContainsKey(languageCode))
        {
            m_dicEventText[languageCode].AddText(textKey, textValue);
        }
        else
        {
            m_dicEventText.Add(languageCode, new TextDictionary(textKey, textValue));
        }

    }

    public string GetText(string textKey)
    {
        var eCurLanguageCode = Language.Instance.GetLanguageCode();
        if (m_dicEventText.ContainsKey(eCurLanguageCode))
        {
            return m_dicEventText[eCurLanguageCode].GetText(textKey);
        }

        return string.Empty;
    }

    public void SendGS_CUSTOM_EVENT_TEXT_GET_REQ() // 로그인 & 환경설정 언어 변경시, 현재 언어의 교환 이벤트 텍스트 Get 요청
    {
        // 이미 저장되어있고 최신화 된 텍스트라면 패스
        var eCurLanguageCode = Language.Instance.GetLanguageCode();
        if (m_dicEventText.ContainsKey(eCurLanguageCode))
        {
            DateTime lastUpdateTime = default;
            if (m_dicLastUpdateEventText.ContainsKey(eCurLanguageCode))
                lastUpdateTime = m_dicLastUpdateEventText[eCurLanguageCode];

            if (lastUpdateTime == LastUpdateTime_EventText)
                return;
        }

        FlatBuffers.FlatBufferBuilder fbb = FlatBuffers.NFlatBufferBuilder.FBB;

        System.Func<object> OffsetMethod = () =>
        {
            return PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TEXT_GET_REQ.CreateGS_CUSTOM_EVENT_TEXT_GET_REQ(fbb, User.Instance.UID, (short)eCurLanguageCode);
        };
        FlatBuffers.NFlatBufferBuilder.SendBytes<PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TEXT_GET_REQ>(PROTOCOL.GAME.ID.ePACKET_ID.GS_CUSTOM_EVENT_TEXT_GET_REQ, OffsetMethod);
    }

    public void RecvGS_CUSTOM_EVENT_TEXT_GET_ACK(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TEXT_GET_ACK ack)
    {
        for (int i = 0; i < ack.TextListLength; i++)
        {
            var pair = ack.GetTextList(i);
            string textKey = TKString.BytesString(pair.GetTextKeyBytes().Value);
            string textValue = TKString.BytesString(pair.GetTextValueBytes().Value);

            AddText(ack.LanguageCode, textKey, textValue);
        }

        // 텍스트 최신화 여부를 저장합니다.
        if (m_dicLastUpdateEventText.ContainsKey(ack.LanguageCode))
            m_dicLastUpdateEventText[ack.LanguageCode] = LastUpdateTime_EventText;
        else
            m_dicLastUpdateEventText.Add(ack.LanguageCode, LastUpdateTime_EventText);

        //Text 받고.. MainTopDlg 최신화 해줘야함.
        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop == null)
            return;

        foreach (var tapType in m_setTapInfo)
            mainTop.Update_EventGroup(tapType, true);
    }

    public void RecvGS_CUSTOM_EVENT_TEXT_UPDATE_NFY(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TEXT_UPDATE_NFY pkt) // 운영툴에서 이벤트 텍스트 update시 현재 언어 갱신해줍니다.
    {
        LastUpdateTime_EventText = DateTime.UtcNow + new TimeSpan(PublicMethod._serverUTCTimeValue);

        for (int i = 0; i < pkt.TextListLength; i++)
        {
            var pair = pkt.GetTextList(i);
            string textKey = TKString.BytesString(pair.GetTextKeyBytes().Value);
            string textValue = TKString.BytesString(pair.GetTextValueBytes().Value);

            AddText(pkt.LanguageCode, textKey, textValue);
        }

        //Text 받고.. MainTopDlg 최신화 해줘야함.
        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop == null)
            return;

        foreach (var tapType in m_setTapInfo)
            mainTop.Update_EventGroup(tapType, true);

    }

    public void RecvGS_CUSTOM_EVENT_PLAY_ACK(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_PLAY_ACK pkt)
    {
        long eventID = pkt.EventID;

        if (false == m_dicEventData.ContainsKey(eventID))
            return;

        if (m_dicEventData[eventID] is BingoEventData bingoData)
            bingoData.m_BingoData.Add(pkt.PlayNumber);
        else if (m_dicEventData[eventID] is RouletteEventData rouletteData)
            rouletteData.Score = pkt.ResultScore;
        else if(m_dicEventData[eventID] is TimingGameEventData timingGameData)
        {
            timingGameData.SucessBar = pkt.SucessPosX;
            timingGameData.GreatSucessBar = pkt.GreatSucessPosX;
            timingGameData.Score = pkt.ResultScore;
        }
        else if(m_dicEventData[eventID] is WarDiceEventData warDiceData)
        {
            warDiceData.DiceBlock = pkt.ResultScore;
        }
                
        int prevRound   = m_dicEventData[eventID].Round;
        int afterRound  = pkt.Round;

        Item mainReward = CustomEventManager.Instance.Get_CustomMainReward(eventID);

        // [1-1] 현재 라운드 보상 획득 여부 갱신
        if (pkt.RewardIDsLength > 0)
        {
            var curRound = GetRoundData(eventID, pkt.Round);
            if (curRound != null)
                curRound.ClearRewarded();

            for (int i = 0; i < pkt.RewardIDsLength; ++i)
            {
                int iLineNumber = pkt.GetRewardIDs(i);
                var rewardData = Get_CustomEventRewardData(eventID, pkt.Round, iLineNumber);
                if (null != rewardData)
                {
                    rewardData.bRewarded = true;
                }
            }
        }

        bool bClearRound    = prevRound != afterRound;
        bool bAllClear      = IsAllClear(eventID); // 혹시 prevRound에서 한방에 마지막 라운드 클리어 가능한 상황이 생긴다면 수정 필요

        // [1-2] 현재 라운드 갱신
        m_dicEventData[eventID].Round = afterRound;

        // 사용한 아이템 소모
        var needItemInfo = BaseTable.TableItemInfo.Instance.Get(pkt.NeedItemKind);
        if (needItemInfo != null)
            Inventory.Instance.AddItemNum(pkt.NeedItemKind, -pkt.NeedItemCount);

        // [2] 보상 획득 처리
        List<ObtainItemBase> addItems = new();

        // 획득 아이템
        for (int i = 0; i < pkt.AddItemsLength; ++i)
        {
            var rewardItem = pkt.GetAddItems(i);
            var itemInfo = BaseTable.TableItemInfo.Instance.Get((int)rewardItem.KIND);
            if (itemInfo == null)
                continue;

            long addItemCount = rewardItem.Count - Inventory.Instance.GetItemNum((int)rewardItem.KIND);

            Inventory.Instance.SetItemNum((int)rewardItem.KIND, rewardItem.Count);
            addItems.Add(new ObtainItemBase((int)rewardItem.KIND, addItemCount));
        }

        // 자원
        var addOil    = pkt.AddAsset.Oil;
        var addIron   = pkt.AddAsset.Iron;
        var addSilver = pkt.AddAsset.Silver;
        var addGold   = pkt.AddAsset.Gold;

        User.Instance.assets.Set(pkt.CurAsset.Oil, pkt.CurAsset.Iron, pkt.CurAsset.Silver, pkt.CurAsset.Gold);

        if (0 < addOil)
            addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eOIL, addOil));
        if (0 < addIron)
            addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eIRON, addIron));
        if (0 < addSilver)
            addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eSILVER, addSilver));
        if (0 < addGold)
            addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eGOLD, addGold));

        // 젬
        long addGem = (pkt.AddVCGem + pkt.AddFreeGem);
        if (0 < addGem)
        {
            NdreamPayment.PaymentManager.Instance.CurGem += addGem;
            User.Instance.assets.Add(0, 0, 0, 0, addGem);
            addItems.Add(new ObtainItemBase(9500, addGem));
        }

        // 쿠폰
        for (int i = 0; i < pkt.CouponsLength; ++i)
        {
            var coupon = pkt.GetCoupons(i);

            CouponManager.Instance.AddCoupon(new Coupon(coupon.UniqueID, coupon.IsActive, coupon.ItemKind, coupon.BeginTime, coupon.EndTime, coupon.DiscountRate,
                TKString.BytesString(coupon.GetProductKindsBytes().Value)));
            addItems.Add(new ObtainItemBase(coupon.ItemKind, 1));
        }

        // 라운드 클리어
        if (bClearRound)
        {
            // 다음 라운드
            if (prevRound < GetMaxRound(eventID))
            {
                // 메인 보상 리셋
                m_dicEventData[eventID].MainRewardItemKind = 0;

                // 빙고판 리셋
                if (m_dicEventData[eventID] is BingoEventData bingoData2)
                    bingoData2.m_BingoData.Clear();
            }
        }

        // 연출 후처리
        void Callback()
        {
            if (addItems.Count > 0 && m_dicEventData[pkt.EventID].EventType != eCUSTUM_EVENT_TYPE.BINGO)
                ObtainItem_ControllerEx.EnQueueEvent(addItems, ObtainEventEx.ContentsType.CONTENTS_COMMON);

            if (false == m_dicEventData.ContainsKey(eventID))
                return;

            if (bClearRound || bAllClear)
            {
                // 라운드 클리어 UI 오픈
                if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.BINGO)
                {
                    // 여기서 연출들 끝났는지 체크하고? 열어주기
                    var bingoDlg = UIFormManager.Instance.FindUIForm<BingoDlg>();
                    if (bingoDlg != null)
                        bingoDlg.TryOpenBingoRewardPopup(prevRound, mainReward);
                }
                else
                {
                    var roundRewardDlg = UIFormManager.Instance.GetUIForm<EventRewardPopupDlg>();
                    if (roundRewardDlg != null)
                        roundRewardDlg.Open(m_dicEventData[eventID].EventType, prevRound, mainReward);
                }
            }
        }

        // [3] UI 갱신
        if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.BINGO)
        {
            var dlg = UIFormManager.Instance.FindUIForm<BingoDlg>();
            if (null != dlg && dlg.Visible)
            {
                Callback();
                dlg.PlayBingo(pkt.PlayNumber);
            }
        }
        else if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.ROULETTE)
        {
            var dlg = UIFormManager.Instance.FindUIForm<LuckyRouletteDlg>();
            if (null != dlg && dlg.Visible)
            {
                dlg.RotatePanel(pkt.PlayNumber, prevRound, Callback);
            }
        }
        else if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.TIMINGGAME)
        {
            var lobbydlg = UIFormManager.Instance.FindUIForm<IntoTheFireLobbyDlg>();
            if (lobbydlg != null && lobbydlg.Visible == true)
                lobbydlg.Refresh();

            var dlg = UIFormManager.Instance.GetUIForm<IntoTheFireDlg>();
            if (dlg != null)
            {
                dlg.SetDifficulty((eCUSTUM_EVENT_DIFFICULTY)pkt.Difficulty);
                dlg.Open();
            }
        }
        else if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.WARDICE)
        {
            void WarDiceCallback()
            {
                if (false == m_dicEventData.ContainsKey(eventID))
                    return;

                if (bClearRound || bAllClear)
                {
                    // 라운드 클리어 UI 오픈
                    var roundRewardDlg = UIFormManager.Instance.GetUIForm<WarDiceRewardPopupDlg>();
                    if (roundRewardDlg != null)
                        roundRewardDlg.Open(m_dicEventData[eventID].EventType, prevRound, mainReward);
                }
            }

            var dlg = UIFormManager.Instance.FindUIForm<WarDiceDlg>();
            if (null != dlg && dlg.Visible)
            {
                dlg.ThrowDice(pkt.PlayNumber, WarDiceCallback);
            }
        }

        // [3] UI 갱신
        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop)
        {
            bool bCalendar = EventCalendarManager.Instance.IsCalendarEvent(eCalendarEvent.CustomEvent, pkt.EventID);
            mainTop.Update_EventGroup(m_dicEventData[pkt.EventID].TapInfo, bCalendar, true);
        }

        // 인벤토리 갱신
        ITEM_INFO_UPDATE.CACHED.Reset();
        ITEM_INFO_UPDATE.CACHED.Set(pkt.NeedItemKind);
        for (int i = 0; i < addItems.Count; ++i)
        {
            ITEM_INFO_UPDATE.CACHED.Set(addItems[i].mItemKind);

        }
         ITEM_INFO_UPDATE.CACHED.Dispatch();

    }
    public Int32 GetQuestRewardAbleCount(Int64 eventID)
    {
        Int32 result = 0;

        Dictionary<Int64, CustomEventQuestData> questDatas = null;

        m_dicQuestData.TryGetValue(eventID, out questDatas);
        if (questDatas == null)
            return result;

        foreach(var (questID, quest) in questDatas)
        {
            if (quest.MissionState == eCUSTOM_EVENT_MISSION_STATE.COMPLETE)
                ++result;
        }

        return result;
    }
    public Int32 GetSuccessBarSize(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {
        Int32 result = 0;

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return result;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return result;

        return valueData.Value6;
    }

    public Int32 GetGreatSuccessBarSize(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {
        Int32 result = 0;

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return result;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return result;

        return valueData.Value7;
    }


    public Int32 GetCurEnemyCount(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {
        Int32 result = 0;

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return result;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return result;

        return valueData.Value4;
    }

    public CustomEventValueData GetValueData(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return null;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return null;

        return valueData;   
    }

    public Int32 GetNeedItemKind(Int64 eventID, eCUSTUM_EVENT_DIFFICULTY Difficulty)
    {
        Int32 result = 0;

        Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> valueDatas = null;

        m_dicValueData.TryGetValue(eventID, out valueDatas);
        if (valueDatas == null)
            return result;

        CustomEventValueData valueData = null;
        valueDatas.TryGetValue(Difficulty, out valueData);

        if (valueData == null)
            return result;

        //타이밍 게임 난이도 NeedItemKind..
        return valueData.Value10;
    }
   
    public void RecvGS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK pkt)
    {
        Int64 eventID = pkt.EventID;

        if (m_dicEventData.ContainsKey(eventID) == false)
            return;


        //아이템 처리..!
        List<ObtainItemBase> addItems = new List<ObtainItemBase>();
        List<Item> rewardItems = new List<Item>();

        if (pkt.RewardIDsLength > 0)
        {
            var curRound = GetRoundData(eventID, pkt.Round);
            if (curRound != null)
                curRound.ClearRewarded();

            for (int i = 0; i < pkt.RewardIDsLength; ++i)
            {
                int iLineNumber = pkt.GetRewardIDs(i);
                var rewardData = Get_CustomEventRewardData(eventID, pkt.Round, iLineNumber);
                if (null != rewardData)
                {
                    rewardData.bRewarded = true;
                }
            }
        }

        // 획득 아이템
        for (int i = 0; i < pkt.ItemsLength; ++i)
        {
            var rewardItem = pkt.GetItems(i);
            var itemInfo = BaseTable.TableItemInfo.Instance.Get((int)rewardItem.ItemKind);
            if (itemInfo == null)
                continue;

            long addItemCount = rewardItem.ItemNum - Inventory.Instance.GetItemNum((int)rewardItem.ItemKind);

            Inventory.Instance.SetItemNum((int)rewardItem.ItemKind, rewardItem.ItemNum);
            addItems.Add(new ObtainItemBase((int)rewardItem.ItemKind, addItemCount));
            rewardItems.Add(new Item((int)rewardItem.ItemKind, addItemCount));
        }

        //자원
        if (pkt.AddAsset != null)
        {
            var addOil = pkt.AddAsset.Oil;
            var addIron = pkt.AddAsset.Iron;
            var addSilver = pkt.AddAsset.Silver;
            var addGold = pkt.AddAsset.Gold;

            User.Instance.assets.Add(addOil, addIron, addSilver, addGold);

            if (0 < addOil)
            {
                addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eOIL, addOil));
                rewardItems.Add(new Item((int)eRESOURCE_TYPE.eOIL, addOil));
            }

            if (0 < addIron)
            {
                addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eIRON, addIron));
                rewardItems.Add(new Item((int)eRESOURCE_TYPE.eIRON, addIron));
            }

            if (0 < addSilver)
            {
                addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eSILVER, addSilver));
                rewardItems.Add(new Item((int)eRESOURCE_TYPE.eSILVER, addSilver));
            }

            if (0 < addGold)
            {
                addItems.Add(new ObtainItemBase((int)eRESOURCE_TYPE.eGOLD, addGold));
                rewardItems.Add(new Item((int)eRESOURCE_TYPE.eGOLD, addGold));
            }
        }

        long addGem = (pkt.AddVCGem + pkt.AddFreeGem);
        if (0 < addGem)
        {
            NdreamPayment.PaymentManager.Instance.CurGem += addGem;
            User.Instance.assets.Add(0, 0, 0, 0, addGem);

            addItems.Add(new ObtainItemBase(CONST_GEM_NOCHARGE, addGem));
            rewardItems.Add(new Item(CONST_GEM_NOCHARGE, addGem));
        }

        for (int i = 0; i < pkt.CouponsLength; ++i)
        {
            var coupon = pkt.GetCoupons(i);

            CouponManager.Instance.AddCoupon(new Coupon(coupon.UniqueID, coupon.IsActive, coupon.ItemKind, coupon.BeginTime, coupon.EndTime, coupon.DiscountRate,
                TKString.BytesString(coupon.GetProductKindsBytes().Value)));
            
            addItems.Add(new ObtainItemBase(coupon.ItemKind, 1));
            rewardItems.Add(new Item(coupon.ItemKind, addGem));
        }

        int prevRound = m_dicEventData[eventID].Round;
        int afterRound = pkt.Round == 0 ? prevRound : pkt.Round;

        Item mainReward = Get_CustomMainReward(eventID);

        m_dicEventData[eventID].Round = afterRound;

        bool bClearRound = prevRound != afterRound;
        bool bAllClear = IsAllClear(eventID);

        if (bClearRound)
        {
            // 다음 라운드
            if (prevRound < GetMaxRound(eventID))
            {
                // 메인 보상 리셋
                m_dicEventData[eventID].MainRewardItemKind = 0;
            }
        }

        void Callback()
        {
            if (addItems.Count > 0)
                ObtainItem_ControllerEx.EnQueueEvent(addItems, ObtainEventEx.ContentsType.CONTENTS_COMMON);

            if (false == m_dicEventData.ContainsKey(eventID))
                return;

            if (bClearRound || bAllClear)
            {
                // 라운드 클리어 UI 오픈
                if (m_dicEventData[pkt.EventID].EventType == eCUSTUM_EVENT_TYPE.BINGO)
                {
                    var roundRewardDlg = UIFormManager.Instance.GetUIForm<BingoRewardPopUpDlg>();
                    if (roundRewardDlg != null)
                        roundRewardDlg.Open(m_dicEventData[eventID].EventType, prevRound, mainReward);
                }
                else
                {
                    var roundRewardDlg = UIFormManager.Instance.GetUIForm<EventRewardPopupDlg>();
                    if (roundRewardDlg != null)
                        roundRewardDlg.Open(m_dicEventData[eventID].EventType, prevRound, mainReward);
                }
            }
        }


        if (m_dicEventData[eventID] is TimingGameEventData timingGameData)
        {
            timingGameData.SucessBar = pkt.SucessPosX;
            timingGameData.GreatSucessBar = pkt.GreatSucessPosX;

            bool endTimingGame = GetCurEnemyCount(m_dicEventData[eventID].EventID, (eCUSTUM_EVENT_DIFFICULTY)pkt.Difficulty) - pkt.Count <= 0;
            if (endTimingGame == true)
            {
                //최종 결과일 때 콜백 등록
                timingGameData.Score += pkt.TotalAddItemCount;
                var lobbydlg = UIFormManager.Instance.GetUIForm<IntotheFireResultDlg>();
                if (lobbydlg != null)
                    lobbydlg.SetCallBack(Callback);
            }
        }


        if (m_dicEventData[eventID].EventType == eCUSTUM_EVENT_TYPE.TIMINGGAME)
        {
            //var lobbydlg = UIFormManager.Instance.FindUIForm<IntoTheFireLobbyDlg>();
            //if (lobbydlg != null && lobbydlg.Visible == true)
            //    lobbydlg.Refresh();

            var dlg = UIFormManager.Instance.FindUIForm<IntoTheFireDlg>();
            if (dlg != null)
            {
                dlg.SetDifficulty((eCUSTUM_EVENT_DIFFICULTY)pkt.Difficulty);
                dlg.AddBonusItemCount(pkt.AddItemCount);
                dlg.SetCurRound(pkt.Count);
                dlg.SetRewardItems(rewardItems);
                dlg.Refresh();
                dlg.CheckCurRoundState();
            }
        }

        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop)
        {
            bool bCalendar = EventCalendarManager.Instance.IsCalendarEvent(eCalendarEvent.CustomEvent, pkt.EventID);
            mainTop.Update_EventGroup(m_dicEventData[pkt.EventID].TapInfo, bCalendar, true);
        }

    }

    public void RecvGS_CUSTOM_EVENT_QUEST_POINT_NFY(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_QUEST_POINT_NFY pkt)
    {
        List<ObtainItemBase> addItems = new List<ObtainItemBase>();

        HashSet<long> eventIDs = new HashSet<long>();

        for (int i = 0; i < pkt.QuestLength; ++i)
        {
            var nfyQuestInfo = pkt.GetQuest(i);
            if (m_dicQuestData.ContainsKey(nfyQuestInfo.EventID) == true)
            {
                var eventQuests = m_dicQuestData[nfyQuestInfo.EventID];
                if (eventQuests.ContainsKey(nfyQuestInfo.QuestID) == true)
                {
                    var curQuest = m_dicQuestData[nfyQuestInfo.EventID][nfyQuestInfo.QuestID];
                    if (curQuest.MissionState == eCUSTOM_EVENT_MISSION_STATE.PROCESSING && nfyQuestInfo.State == (int)eCUSTOM_EVENT_MISSION_STATE.COMPLETE)
                    {
                        var eventData = GetCustomEventData(nfyQuestInfo.EventID);
                        if(eventData != null)
                        {
                            EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(new EVENT.PopUpSetBuffDlgDirection(NTextManager.Instance.GetText(curQuest.PointHelpKey), NTextManager.Instance.GetText("NOTICE_TIMINGGAME_MISSION_COMPLETE")));
                        }
                    }

                    curQuest.EventID = nfyQuestInfo.EventID;
                    curQuest.QuestID = nfyQuestInfo.QuestID;
                    curQuest.ConditionKind = nfyQuestInfo.ConditionKind;
                    curQuest.ConditionValue = nfyQuestInfo.ConditionValue;
                    curQuest.CurValue = nfyQuestInfo.CurValue;
                    curQuest.MissionState = (eCUSTOM_EVENT_MISSION_STATE)nfyQuestInfo.State;
                    curQuest.TargetValue = nfyQuestInfo.TargetValue;
                    curQuest.PointHelpKey = TKString.BytesString(nfyQuestInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");
                }
                else
                {
                    var questData = new CustomEventQuestData();

                    questData.EventID = nfyQuestInfo.EventID;
                    questData.QuestID = nfyQuestInfo.QuestID;
                    questData.ConditionKind = nfyQuestInfo.ConditionKind;
                    questData.ConditionValue = nfyQuestInfo.ConditionValue;
                    questData.CurValue = nfyQuestInfo.CurValue;
                    questData.MissionState = (eCUSTOM_EVENT_MISSION_STATE)nfyQuestInfo.State;
                    questData.TargetValue = nfyQuestInfo.TargetValue;
                    questData.PointHelpKey = TKString.BytesString(nfyQuestInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");
                }
            }
            else
            {
                Dictionary<Int64, CustomEventQuestData> dicQuestData = new Dictionary<long, CustomEventQuestData>();

                CustomEventQuestData questData = new CustomEventQuestData();

                questData.EventID = nfyQuestInfo.EventID;
                questData.QuestID = nfyQuestInfo.QuestID;
                questData.ConditionKind = nfyQuestInfo.ConditionKind;
                questData.ConditionValue = nfyQuestInfo.ConditionValue;
                questData.CurValue = nfyQuestInfo.CurValue;
                questData.TargetValue = nfyQuestInfo.TargetValue;
                questData.MissionState = (eCUSTOM_EVENT_MISSION_STATE)nfyQuestInfo.State;
                questData.PointHelpKey = TKString.BytesString(nfyQuestInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");

                dicQuestData.Add(questData.QuestID, questData);

                m_dicQuestData.Add(questData.EventID, dicQuestData);
            }

            if (eventIDs.Contains(nfyQuestInfo.EventID) == false)
                eventIDs.Add(nfyQuestInfo.EventID);

            for (int j = 0; j < nfyQuestInfo.QuestRewardItemsLength; ++j)
            {
                var itemInfo = nfyQuestInfo.GetQuestRewardItems(j);
                if (itemInfo == null)
                    continue;

                Inventory.Instance.AddItemNum((int)itemInfo.KIND, itemInfo.Count);
                addItems.Add(new ObtainItemBase((int)itemInfo.KIND, itemInfo.Count));
            }
        }

        // 아이템 획득 연출
        if (addItems.Count > 0)
            ObtainItem_ControllerEx.EnQueueEvent(addItems, ObtainEventEx.ContentsType.CONTENTS_COMMON);

        //var lobbyDlg = UIFormManager.Instance.FindUIForm<IntoTheFireLobbyDlg>();
        //if (lobbyDlg != null && lobbyDlg.Visible == true)
        //    lobbyDlg.Refresh();

        var missionDlg = UIFormManager.Instance.FindUIForm<IntoTheFireMissionDlg>();
        if (missionDlg != null && missionDlg.Visible == true)
            missionDlg.Refresh();

        var lobbyDlg = UIFormManager.Instance.FindUIForm<IntoTheFireLobbyDlg>();
        if (lobbyDlg != null && lobbyDlg.Visible == true)
            lobbyDlg.Refresh();

        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop)
        {
            foreach (var eventID in eventIDs)
            {
                bool bCalendar = EventCalendarManager.Instance.IsCalendarEvent(eCalendarEvent.CustomEvent, eventID);
                if (m_dicEventData.ContainsKey(eventID))
                    mainTop.Update_EventGroup(m_dicEventData[eventID].TapInfo, bCalendar, true);
            }
        }

    }

    public void RecvGS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK pkt)
    {
        if (false == m_dicEventData.ContainsKey(pkt.EventID))
            return;

        var curEvent = m_dicEventData[pkt.EventID];
        if (null == curEvent)
            return;

        curEvent.MainRewardItemKind = pkt.ChangeItemKind;

        if (curEvent.EventType == eCUSTUM_EVENT_TYPE.BINGO)
        {
            var dlg = UIFormManager.Instance.FindUIForm<BingoDlg>();
            if (null != dlg && dlg.Visible)
                dlg.RefreshMainReward();
        }
        else if (curEvent.EventType == eCUSTUM_EVENT_TYPE.ROULETTE)
        {
            var dlg = UIFormManager.Instance.FindUIForm<LuckyRouletteDlg>();
            if (null != dlg && dlg.Visible)
                dlg.RefreshMainReward();
        }
        else if(curEvent.EventType == eCUSTUM_EVENT_TYPE.TIMINGGAME)
        {
            var dlg = UIFormManager.Instance.FindUIForm<IntoTheFireLobbyDlg>();
            if (null != dlg && dlg.Visible)
                dlg.SetMainReward();
        }
        else if (curEvent.EventType == eCUSTUM_EVENT_TYPE.WARDICE)
        {
            var dlg = UIFormManager.Instance.FindUIForm<WarDiceDlg>();
            if (null != dlg && dlg.Visible)
                dlg.RefreshMainReward();
        }

        if (true == UIFormManager.Instance.IsOpenUIForm<SelectRewardPopUp>())
            UIFormManager.Instance.CloseUIForm<SelectRewardPopUp>();

        if (true == UIFormManager.Instance.IsOpenUIForm<BingoSelectRewardDlg>())
            UIFormManager.Instance.CloseUIForm<BingoSelectRewardDlg>();

        // [3] UI 갱신
        var mainTop = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (mainTop)
        {
            bool bCalendar = EventCalendarManager.Instance.IsCalendarEvent(eCalendarEvent.CustomEvent, pkt.EventID);
            mainTop.Update_EventGroup(m_dicEventData[pkt.EventID].TapInfo, bCalendar, true);
        }
    }

    #region EventInfoUpdate
    public void RecvGS_CUSTOM_EVENT_INFO_NFY(PROTOCOL.FLATBUFFERS.GS_CUSTOM_EVENT_INFO_NFY pkt)
    {
        // Add Event Info
        for (int i = 0; i < pkt.EventInfosLength; i++)
        {
            var eventInfo = pkt.GetEventInfos(i);

            if (m_dicEventData.ContainsKey(eventInfo.EventID))
            {
                m_dicEventData[eventInfo.EventID].EventID            = eventInfo.EventID;
                m_dicEventData[eventInfo.EventID].EventType          = (eCUSTUM_EVENT_TYPE)eventInfo.EventType;
                m_dicEventData[eventInfo.EventID].ScheduleType       = (eCUSTUM_EVENT_SCHEDULE_TYPE)eventInfo.ScheduleType;
                m_dicEventData[eventInfo.EventID].QuestGroup         = eventInfo.GroupID;
                m_dicEventData[eventInfo.EventID].StartTime          = eventInfo.StartTime;
                m_dicEventData[eventInfo.EventID].EndTime            = eventInfo.EndTime;
                m_dicEventData[eventInfo.EventID].KeyItemKind        = eventInfo.KeyItemKind;
                m_dicEventData[eventInfo.EventID].KeyItemNum         = eventInfo.KeyItemCount;
                m_dicEventData[eventInfo.EventID].MainRewardItemKind = eventInfo.RewardKind;
                m_dicEventData[eventInfo.EventID].TapInfo            = (eBTN_GROUP_TYPE)eventInfo.TapInfo;

                m_dicEventData[eventInfo.EventID].IconKey            = TKString.BytesString(eventInfo.GetIconKeyBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].IconSprite         = TKString.BytesString(eventInfo.GetIconPathBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].IconAtlas          = TKString.BytesString(eventInfo.GetIconAtlasBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].TitleKey           = TKString.BytesString(eventInfo.GetTitleKeyBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].HelpKey            = TKString.BytesString(eventInfo.GetHelpKeyBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].BGImage            = TKString.BytesString(eventInfo.GetBGImagePathBytes().Value).Replace(" ", "");
                m_dicEventData[eventInfo.EventID].NumberBGImage      = TKString.BytesString(eventInfo.GetNumberImagePathBytes().Value).Replace(" ", "");

                if (m_dicEventData[eventInfo.EventID] is BingoEventData bingoData)
                {
                    bingoData.m_BingoData.Clear();
                    for (int j = 0; j < eventInfo.ProgressLength; ++j)
                        bingoData.m_BingoData.Add(eventInfo.GetProgress(j));
                }
                else if (m_dicEventData[eventInfo.EventID] is RouletteEventData rouletteData)
                {
                    if (eventInfo.ProgressLength > 0)
                        rouletteData.Score = eventInfo.GetProgress(0);
                }
                else if (m_dicEventData[eventInfo.EventID] is TimingGameEventData TimingGameData)
                {
                    if (eventInfo.ProgressLength > 0)
                        TimingGameData.Score = eventInfo.GetProgress(0);
                }
                else if(m_dicEventData[eventInfo.EventID] is WarDiceEventData warDiceData)
                {
                    if (eventInfo.ProgressLength > 0)
                        warDiceData.DiceBlock = eventInfo.GetProgress(0);
                }

                m_dicEventData[eventInfo.EventID].Round             = eventInfo.Round;
            }
            else
            {
                CustomEventData eventData = null;
                switch ((eCUSTUM_EVENT_TYPE)eventInfo.EventType)
                {
                    case eCUSTUM_EVENT_TYPE.BINGO:    eventData = new BingoEventData(); break;
                    case eCUSTUM_EVENT_TYPE.ROULETTE: eventData = new RouletteEventData(); break;
                    case eCUSTUM_EVENT_TYPE.TIMINGGAME: eventData = new TimingGameEventData(); break;
                    case eCUSTUM_EVENT_TYPE.WARDICE: eventData = new WarDiceEventData(); break;
                    default: continue;
                }

                eventData.EventID            = eventInfo.EventID;
                eventData.EventType          = (eCUSTUM_EVENT_TYPE)eventInfo.EventType;
                eventData.ScheduleType       = (eCUSTUM_EVENT_SCHEDULE_TYPE)eventInfo.ScheduleType;
                eventData.StartTime          = eventInfo.StartTime;
                eventData.EndTime            = eventInfo.EndTime;
                eventData.KeyItemKind        = eventInfo.KeyItemKind;
                eventData.KeyItemNum         = eventInfo.KeyItemCount;
                eventData.MainRewardItemKind = eventInfo.RewardKind;
                eventData.TapInfo            = (eBTN_GROUP_TYPE)eventInfo.TapInfo;

                eventData.IconKey            = TKString.BytesString(eventInfo.GetIconKeyBytes().Value).Replace(" ", "");
                eventData.IconSprite         = TKString.BytesString(eventInfo.GetIconPathBytes().Value).Replace(" ", "");
                eventData.IconAtlas          = TKString.BytesString(eventInfo.GetIconAtlasBytes().Value).Replace(" ", "");
                eventData.TitleKey           = TKString.BytesString(eventInfo.GetTitleKeyBytes().Value).Replace(" ", "");
                eventData.HelpKey            = TKString.BytesString(eventInfo.GetHelpKeyBytes().Value).Replace(" ", "");
                eventData.BGImage            = TKString.BytesString(eventInfo.GetBGImagePathBytes().Value).Replace(" ", "");
                eventData.NumberBGImage      = TKString.BytesString(eventInfo.GetNumberImagePathBytes().Value).Replace(" ", "");

                if (eventData is BingoEventData bingoData)
                {
                    for (int j = 0; j < eventInfo.ProgressLength; ++j)
                        bingoData.m_BingoData.Add(eventInfo.GetProgress(j));
                }
                else if (eventData is RouletteEventData rouletteData)
                {
                    if (eventInfo.ProgressLength > 0)
                        rouletteData.Score = eventInfo.GetProgress(0);
                }
                else if (eventData is TimingGameEventData TimingGameData)
                {
                    if (eventInfo.ProgressLength > 0)
                        TimingGameData.Score = eventInfo.GetProgress(0);
                }
                else if (eventData is WarDiceEventData warDiceData)
                {
                    if (eventInfo.ProgressLength > 0)
                        warDiceData.DiceBlock = eventInfo.GetProgress(0);
                }

                eventData.Round             = eventInfo.Round;

                if (eventData.TapInfo != eBTN_GROUP_TYPE.NONE && eventData.TapInfo != eBTN_GROUP_TYPE.MAX)
                    m_setTapInfo.Add(eventData.TapInfo);

                m_dicEventData.Add(eventInfo.EventID, eventData);
            }
        }

        // Add Round Info
        for (int i = 0; i < pkt.RoundInfosLength; i++)
        {
            var roundInfo = pkt.GetRoundInfos(i);

            CustomEventRoundData roundData = new CustomEventRoundData();

            roundData.EventID    = roundInfo.EventID;
            roundData.Round      = roundInfo.Round;
            roundData.CustomImage = TKString.BytesString(roundInfo.GetCustomImagePathBytes().Value).Replace(" ", "");

            roundData.RoundGoal.Clear();
            for (int k = 0; k < roundInfo.RoundGoalLength; ++k)
                roundData.RoundGoal.Add(roundInfo.GetRoundGoal(k));

            var curEvent = GetCustomEventData(roundData.EventID);
            if(curEvent != null && curEvent.EventType != eCUSTUM_EVENT_TYPE.WARDICE)
                roundData.RoundGoal.Sort((int a, int b) => { return a.CompareTo(b); });

               // Reward Info
            roundData.Rewards.Clear();
            for (int j = 0; j < roundInfo.RewardItemsLength; ++j)
            {
                var rewardInfo = roundInfo.GetRewardItems(j);

                CustomEventRewardData rewardData = new CustomEventRewardData();

                rewardData.EventID          = rewardInfo.EventID;
                rewardData.RewardGroupID    = rewardInfo.RewardGroupID;
                rewardData.bRewarded        = rewardInfo.Rewarded;
                rewardData.RewardType       = rewardInfo.RewardType;
                rewardData.Round            = roundData.Round;

                rewardData.Rewards.Clear();
                for (int k = 0; k < rewardInfo.RewardItemsLength; ++k)
                {
                    var item = rewardInfo.GetRewardItems(k);
                    rewardData.Rewards.Add(new Item((int)item.KIND, item.Count));
                }

#if UNITY_EDITOR
                if (roundData.Rewards.ContainsKey(rewardInfo.RewardType))
                {
                    NDebug.LogError($"Already Exist LineNumber : {rewardInfo.EventID}-{roundInfo.Round}-{rewardInfo.RewardGroupID}-{rewardInfo.RewardType}");
                }
#endif

                if (false == roundData.Rewards.ContainsKey(rewardInfo.RewardType))
                    roundData.Rewards.Add(rewardInfo.RewardType, rewardData);
                else
                    roundData.Rewards[rewardInfo.RewardType].Rewards.AddRange(rewardData.Rewards);
            }

            if (false == m_dicEvent_RoundData.ContainsKey(roundInfo.EventID))
            {
                Dictionary<int, CustomEventRoundData> dicData = new Dictionary<int, CustomEventRoundData>();
                m_dicEvent_RoundData.Add(roundInfo.EventID, dicData);
            }

            if (false == m_dicEvent_RoundData[roundInfo.EventID].ContainsKey(roundInfo.Round))
                m_dicEvent_RoundData[roundInfo.EventID].Add(roundInfo.Round, roundData);
            else
                m_dicEvent_RoundData[roundInfo.EventID][roundInfo.Round] = roundData;
        }

        // Add Probability Info
        for (int i = 0; i < pkt.ProbabilityInfosLength; i++)
        {
            var probabilityInfo = pkt.GetProbabilityInfos(i);

            CustomEventProbabilityData probabilityData = new CustomEventProbabilityData();

            probabilityData.EventID = probabilityInfo.EventID;
            probabilityData.ProbKey_1 = probabilityInfo.ProbKey1;
            probabilityData.ProbKey_2 = probabilityInfo.ProbKey2;

            if (false == m_dicEvent_ProbabilityData.ContainsKey(probabilityInfo.EventID))
            {
                SortedSet<CustomEventProbabilityData> setData = new SortedSet<CustomEventProbabilityData>();
                m_dicEvent_ProbabilityData.Add(probabilityInfo.EventID, setData);
            }

            m_dicEvent_ProbabilityData[probabilityInfo.EventID].Add(probabilityData);
        }

        // Add Quest Info
        for (int i = 0; i < pkt.QuestInfosLength; i++)
        {
            var questInfo = pkt.GetQuestInfos(i);

            if (m_dicQuestData.ContainsKey(questInfo.EventID) == true)
            {
                var quests = m_dicQuestData[questInfo.EventID];
                if(quests.ContainsKey(questInfo.QuestID)== true)
                {
                    var curQuest = m_dicQuestData[questInfo.EventID][questInfo.QuestID];

                    curQuest.EventID = questInfo.EventID;
                    curQuest.QuestID = questInfo.QuestID;
                    curQuest.ConditionKind = questInfo.ConditionKind;
                    curQuest.ConditionValue = questInfo.ConditionValue;
                    curQuest.CurValue = questInfo.CurValue;
                    curQuest.TargetValue = questInfo.TargetValue;
                    curQuest.MissionState = (eCUSTOM_EVENT_MISSION_STATE)questInfo.State;
                    curQuest.PointHelpKey = TKString.BytesString(questInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");

                    curQuest.RewardItems.Clear();

                    for (int j = 0; j < questInfo.QuestRewardItemsLength; ++j)
                    {
                        var rewardItem = questInfo.GetQuestRewardItems(j);
                        curQuest.RewardItems.Add(new Item((int)rewardItem.KIND, rewardItem.Count));
                    }
                }
                else
                {
                    CustomEventQuestData questData = new CustomEventQuestData();

                    questData.EventID = questInfo.EventID;
                    questData.QuestID = questInfo.QuestID;
                    questData.ConditionKind = questInfo.ConditionKind;
                    questData.ConditionValue = questInfo.ConditionValue;
                    questData.CurValue = questInfo.CurValue;
                    questData.TargetValue = questInfo.TargetValue;
                    questData.MissionState = (eCUSTOM_EVENT_MISSION_STATE)questInfo.State;
                    questData.PointHelpKey = TKString.BytesString(questInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");

                    questData.RewardItems.Clear();

                    for (int j = 0; j < questInfo.QuestRewardItemsLength; ++j)
                    {
                        var rewardItem = questInfo.GetQuestRewardItems(j);
                        questData.RewardItems.Add(new Item((int)rewardItem.KIND, rewardItem.Count));
                    }
                    m_dicQuestData[questInfo.EventID].Add(questInfo.QuestID, questData);

                }
            }
            else
            {
                Dictionary<Int64, CustomEventQuestData> dicQuestData = new Dictionary<long, CustomEventQuestData>();

                CustomEventQuestData questData = new CustomEventQuestData();

                questData.EventID = questInfo.EventID;
                questData.QuestID = questInfo.QuestID;
                questData.ConditionKind = questInfo.ConditionKind;
                questData.ConditionValue = questInfo.ConditionValue;
                questData.CurValue = questInfo.CurValue;
                questData.TargetValue = questInfo.TargetValue;
                questData.MissionState = (eCUSTOM_EVENT_MISSION_STATE)questInfo.State;
                questData.PointHelpKey = TKString.BytesString(questInfo.GetPointHelpKeyBytes().Value).Replace(" ", "");

                questData.RewardItems.Clear();

                for (int j = 0; j < questInfo.QuestRewardItemsLength; ++j)
                {
                    var rewardItem = questInfo.GetQuestRewardItems(j);
                    questData.RewardItems.Add(new Item((int)rewardItem.KIND, rewardItem.Count));
                }

                dicQuestData.Add(questData.QuestID, questData);

                m_dicQuestData.Add(questData.EventID, dicQuestData);
            }
        }

        // Add Value Info
        for (int i = 0; i < pkt.ValueInfosLength; i++)
        {
            var valueInfo = pkt.GetValueInfos(i);

            if (m_dicValueData.ContainsKey(valueInfo.EventID))
            {
                var myValueInfo = m_dicValueData[valueInfo.EventID];

                if (myValueInfo.ContainsKey((eCUSTUM_EVENT_DIFFICULTY)valueInfo.Difficulty))
                {
                    var valueData = m_dicValueData[valueInfo.EventID][(eCUSTUM_EVENT_DIFFICULTY)valueInfo.Difficulty];

                    valueData.EventID = valueInfo.EventID;
                    valueData.Difficulty = valueInfo.Difficulty;
                    valueData.Value1 = valueInfo.Value1;
                    valueData.Value2 = valueInfo.Value2;
                    valueData.Value3 = valueInfo.Value3;
                    valueData.Value4 = valueInfo.Value4;
                    valueData.Value5 = valueInfo.Value5;
                    valueData.Value6 = valueInfo.Value6;
                    valueData.Value7 = valueInfo.Value7;
                    valueData.Value8 = valueInfo.Value8;
                    valueData.Value9 = valueInfo.Value9;
                    valueData.Value10 = valueInfo.Value10;
                }
                else
                {
                    CustomEventValueData valueData = new CustomEventValueData();

                    valueData.EventID = valueInfo.EventID;
                    valueData.Difficulty = valueInfo.Difficulty;
                    valueData.Value1 = valueInfo.Value1;
                    valueData.Value2 = valueInfo.Value2;
                    valueData.Value3 = valueInfo.Value3;
                    valueData.Value4 = valueInfo.Value4;
                    valueData.Value5 = valueInfo.Value5;
                    valueData.Value6 = valueInfo.Value6;
                    valueData.Value7 = valueInfo.Value7;
                    valueData.Value8 = valueInfo.Value8;
                    valueData.Value9 = valueInfo.Value9;
                    valueData.Value10 = valueInfo.Value10;

                    m_dicValueData[valueInfo.EventID].Add((eCUSTUM_EVENT_DIFFICULTY)valueInfo.Difficulty, valueData);
                }
            }
            else
            {

                Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData> dicValueData = new Dictionary<eCUSTUM_EVENT_DIFFICULTY, CustomEventValueData>();

                CustomEventValueData valueData = new CustomEventValueData();

                valueData.EventID = valueInfo.EventID;
                valueData.Difficulty = valueInfo.Difficulty;
                valueData.Value1 = valueInfo.Value1;
                valueData.Value2 = valueInfo.Value2;
                valueData.Value3 = valueInfo.Value3;
                valueData.Value4 = valueInfo.Value4;
                valueData.Value5 = valueInfo.Value5;
                valueData.Value6 = valueInfo.Value6;
                valueData.Value7 = valueInfo.Value7;
                valueData.Value8 = valueInfo.Value8;
                valueData.Value9 = valueInfo.Value9;
                valueData.Value10 = valueInfo.Value10;

                dicValueData.Add((eCUSTUM_EVENT_DIFFICULTY)valueInfo.Difficulty, valueData);
                m_dicValueData.Add(valueInfo.EventID, dicValueData);
            }
        }


    }


    #endregion
}
