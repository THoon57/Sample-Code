using System;
using System.Collections.Generic;
using DEFINE;
using UnityEngine;
using BaseTable;
using GameEvent;
using NdreamPayment;
using NLibCs;
using static DEFINE.GameDefine;


public enum eBattlePassRewardType
{
    NONE = 0,
    FREE = 1,
    PAID = 2,
    END,
}
public enum eBattlePassMissionState
{
    NONE = 0,
    PROCESSING = 1,
    COMPLETE = 2,
    REWARDED = 3,
    EXPIRE = 4,
    END,
}
public enum eBattlePassPremiumRewardType
{
    NONE = 0,
    PREMIUM_SHOW = 1,
    PLUS_TIER = 2,
}

public enum eBattlePassInvalidUseReason
{
    NONE = 0,
    ROUND_BEFORE = 1,
    CHECK_PAIDPASS = 2,
};


public class BattlePassUser
{
    public Int32 PassKind = 0;
    public Int32 Round = 0;
    public Int32 RewardGroup = 0;
    public Int64 StartTime = 0;
    public Int64 EndTime = 0;
    public eBattlePassType PassType = eBattlePassType.NONE;

    private Int64 CurPassPoint = 0;
    public long Point
    {
        set => CurPassPoint = value;

        get
        {
            var maxTier = BattlePassManager.Instance.GetMaxTier(PassKind);
            var curTier = CurPassPoint / BattlePassManager.CONST_ONE_TIER;

            return curTier >= maxTier ? (maxTier * BattlePassManager.CONST_ONE_TIER) : CurPassPoint;
        }
    }

    public bool isPaid => PassType == eBattlePassType.PAID;
}

public class BattlePassMission
{
    public Int32 PassKind = 0;
    public Int64 Missionkind = 0;
    public Int64 CurValue = 0;
    public eBattlePassMissionState MissionState = eBattlePassMissionState.NONE;

    public Int32 GetStatePriority()
    {
        Int32 priority = Int32.MaxValue;

        var missionInfo = TableBattlePass_Mission.Instance.GetMission(PassKind, Missionkind);
        if (missionInfo == null)
            return priority;

        switch (MissionState)
        {
            // 일반 미션 먼저 보여주고, 프리미엄 보상 다음에
            case eBattlePassMissionState.COMPLETE: priority = missionInfo.isPaidMission ? 1 : 0; break;
            case eBattlePassMissionState.PROCESSING: priority = 2; break;
            case eBattlePassMissionState.REWARDED: priority = 3; break;
        }

        return priority;
    }
}
public class BattlePassManager : NrTSingleton<BattlePassManager>
{
    const int DAY_DURATION = 60 * 60 * 24;
    const int WEEK_DURATION = DAY_DURATION * 7;

    public const Int64 CONST_ONE_TIER = 100;

    //Kind, User
    public Dictionary<Int32, BattlePassUser> m_User = new Dictionary<int, BattlePassUser>();
    //(Kind, (Tier, Type))
    public Dictionary<Int32, HashSet<(Int32, eBattlePassRewardType)>> m_Reward = new Dictionary<int, HashSet<(int, eBattlePassRewardType)>>();
    //(Kind ,(MissionKind,Mission))
    private Dictionary<Int32, Dictionary<Int64, BattlePassMission>> m_Mission = new Dictionary<int, Dictionary<long, BattlePassMission>>();

    // 10티어 이하에서 패스포인트 아이템 사용시, 경고문을 1회 보여줍니다.
    public static bool _checkItemUsePopup = false;

    private BattlePassManager()
    {
    }
    public void Clear()
    {
        m_User.Clear();
        m_Mission.Clear();
        m_Reward.Clear();
        _checkItemUsePopup = false;
    }

    // 앞으로 레드닷에 포함하지 않을 미션들
    public void SetMissionTypeIgnore(int passKind, eBattlePassMissionType missionScheduleType, int missionScheduleValue, bool ignoreMarking)
    {
        m_User.TryGetValue(passKind, out BattlePassUser userPassData);
        if (userPassData == null)
            return;

        // 패스 구매 유저라면 PlayerPrefs 더 이상 사용할 필요 없다.
        if (userPassData.isPaid)
            return;

        PlayerPrefs_PrimePassMissionReward.SetValue(User.Instance.UID, userPassData.PassKind, (int) missionScheduleType, missionScheduleValue, ignoreMarking);
    }

    public bool GetMissionIgnoreType(int passKind, eBattlePassMissionType missionScheduleType, int missionScheduleValue, bool isPaidMission)
    {
        m_User.TryGetValue(passKind, out BattlePassUser userPassData);
        if (userPassData == null)
            return false;

        // 패스 구매 유저라면 PlayerPrefs 검사하지 않고 원래 레드닷 규칙을 사용하낟.
        if (userPassData.isPaid)
            return false;

        if (!isPaidMission)
            return false;

        return PlayerPrefs_PrimePassMissionReward.GetValue(User.Instance.UID, userPassData.PassKind, (int)missionScheduleType, missionScheduleValue);
    }


    public bool CheckCanUserPointItem(NrItemDataInfo itemInfo, out eBattlePassInvalidUseReason reason)
    {
        reason = eBattlePassInvalidUseReason.NONE;

        if (itemInfo == null)
            return false;

        var passKind = itemInfo.itemvalue4;

        var passInfo = TableBattlePass_Schedule.Instance.GetPassSchedule(passKind);
        if (passInfo == null)
        {
            reason = eBattlePassInvalidUseReason.NONE;
            return false;
        }

        var curPass = GetBattlePassUser(passInfo.PassKind);
        if (curPass == null)
        {
            reason = eBattlePassInvalidUseReason.ROUND_BEFORE;
            return false;
        }

        if (curPass.PassType == eBattlePassType.FREE && _checkItemUsePopup == false)
        {
            reason = eBattlePassInvalidUseReason.CHECK_PAIDPASS;
            return false;
        }

        return true;
    }


    public void RECV_GS_BATTLEPASS_INFO_NFY(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_INFO_NFY pkt)
    {
        Clear();

        for(int i = 0; i < pkt.UserLength; ++i)
        {
            var userInfo = pkt.GetUser(i);
            if(m_User.ContainsKey(userInfo.Kind))
            {
                var userBattlePass = m_User[userInfo.Kind];

                userBattlePass.PassKind           = userInfo.Kind;
                userBattlePass.Round              = userInfo.Round;
                userBattlePass.RewardGroup        = userInfo.RewardGroup;
                userBattlePass.Point              = userInfo.Point;
                userBattlePass.StartTime          = userInfo.PassStartTime;
                userBattlePass.EndTime            = userInfo.PassEndTime;
                userBattlePass.PassType           = (eBattlePassType)userInfo.PassType;
            }
            else
            {
                var userData = new BattlePassUser();
                userData.PassKind            = userInfo.Kind;
                userData.Round               = userInfo.Round;
                userData.RewardGroup         = userInfo.RewardGroup;
                userData.Point               = userInfo.Point;
                userData.StartTime           = userInfo.PassStartTime;
                userData.EndTime             = userInfo.PassEndTime;
                userData.PassType            = (eBattlePassType)userInfo.PassType;

                m_User.Add(userInfo.Kind, userData);
            }
        }
        for(int i = 0; i < pkt.RewardLength; ++i)
        {
            var rewardInfo = pkt.GetReward(i);
            if (m_Reward.ContainsKey(rewardInfo.Kind))
            {
                if(m_Reward[rewardInfo.Kind].Contains((rewardInfo.Tier, (eBattlePassRewardType)rewardInfo.RewardType)) == false)
                {
                    m_Reward[rewardInfo.Kind].Add((rewardInfo.Tier, (eBattlePassRewardType)rewardInfo.RewardType));
                }
            }
            else
            {
                var rewardHash = new HashSet<(Int32, eBattlePassRewardType)>();
                rewardHash.Add((rewardInfo.Tier, (eBattlePassRewardType)rewardInfo.RewardType));
                m_Reward.Add(rewardInfo.Kind, rewardHash);
            }
        }
        for(int i = 0; i < pkt.MissionLength; ++i)
        {
            var pktMissionInfo = pkt.GetMission(i);

            if (pktMissionInfo.State == (int)eBattlePassMissionState.EXPIRE)
                continue;

            if (m_Mission.ContainsKey(pktMissionInfo.Kind))
            {
                var passMissions = m_Mission[pktMissionInfo.Kind];

                if (passMissions.ContainsKey(pktMissionInfo.MissionKind))
                {
                    var targetMission = m_Mission[pktMissionInfo.Kind][pktMissionInfo.MissionKind];

                    targetMission.PassKind       = pktMissionInfo.Kind;
                    targetMission.Missionkind    = pktMissionInfo.MissionKind;
                    targetMission.CurValue       = pktMissionInfo.CurValue;
                    targetMission.MissionState   = (eBattlePassMissionState)pktMissionInfo.State;
                }
                else
                {
                    var missionData = new BattlePassMission();
                    missionData.PassKind         = pktMissionInfo.Kind;
                    missionData.Missionkind      = pktMissionInfo.MissionKind;
                    missionData.CurValue         = pktMissionInfo.CurValue;
                    missionData.MissionState     = (eBattlePassMissionState)pktMissionInfo.State;

                    m_Mission[pktMissionInfo.Kind].Add(pktMissionInfo.MissionKind, missionData);
                }
            }
            else
            {
                Dictionary<Int64, BattlePassMission> dicMissionData = new Dictionary<long, BattlePassMission>();

                var missionData = new BattlePassMission();

                missionData.PassKind = pktMissionInfo.Kind;
                missionData.Missionkind = pktMissionInfo.MissionKind;
                missionData.CurValue = pktMissionInfo.CurValue;
                missionData.MissionState = (eBattlePassMissionState)pktMissionInfo.State;

                dicMissionData.Add(pktMissionInfo.MissionKind, missionData);

                m_Mission.Add(pktMissionInfo.Kind, dicMissionData);
            }
        }

        //배틀 패스 UI Refresh 함수 호출!!

    }

    public void RECV_GS_BATTLEPASS_MISSION_COMPLETE_ACK(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_MISSION_COMPLETE_ACK pkt)
    {
        var result = pkt.Result;

        var passKind = pkt.PassKind;
        var passPoint = pkt.ResultPoint;
        var completeMission = pkt.Mission;
        var curTier = GetCurTier((int)passKind);
        var resultTier = passPoint / BattlePassManager.CONST_ONE_TIER;

        // 보상 지급
        {
            List<ObtainItemBase> addItems = new List<ObtainItemBase>();
            // 아이템
            for (int i = 0; i < pkt.RewardItemLength; ++i)
            {
                var rewardItem = pkt.GetRewardItem(i);
                var itemInfo = BaseTable.TableItemInfo.Instance.Get((int)rewardItem.KIND);
                if (itemInfo == null)
                    continue;

                if (false == itemInfo.itemDirectUse)
                    Inventory.Instance.AddItemNum((int)rewardItem.KIND, rewardItem.Count);

                addItems.Add(new ObtainItemBase((int)rewardItem.KIND, rewardItem.Count));
            }

            if (addItems.Count > 0)
                ObtainItem_ControllerEx.EnQueueEvent(addItems);
        }

        // 미션 갱신
        if (m_Mission.ContainsKey(completeMission.Kind))
        {
            var passMissions = m_Mission[completeMission.Kind];

            if (passMissions.ContainsKey(completeMission.MissionKind))
            {
                var targetMission = m_Mission[completeMission.Kind][completeMission.MissionKind];

                targetMission.PassKind = completeMission.Kind;
                targetMission.Missionkind = completeMission.MissionKind;
                targetMission.CurValue = completeMission.CurValue;
                targetMission.MissionState = (eBattlePassMissionState)completeMission.State;
            }
            else
            {
                var missionData = new BattlePassMission();
                missionData.PassKind = completeMission.Kind;
                missionData.Missionkind = completeMission.MissionKind;
                missionData.CurValue = completeMission.CurValue;
                missionData.MissionState = (eBattlePassMissionState)completeMission.State;

                m_Mission[completeMission.Kind].Add(completeMission.MissionKind, missionData);
            }
        }
        else
        {
            Dictionary<Int64, BattlePassMission> dicMissionData = new Dictionary<long, BattlePassMission>();

            var missionData = new BattlePassMission();

            missionData.PassKind = completeMission.Kind;
            missionData.Missionkind = completeMission.MissionKind;
            missionData.CurValue = completeMission.CurValue;
            missionData.MissionState = (eBattlePassMissionState)completeMission.State;

            dicMissionData.Add(completeMission.MissionKind, missionData);

            m_Mission.Add(completeMission.Kind, dicMissionData);
        }

        var addPoint = passPoint - m_User[completeMission.Kind].Point;
        //기존 포인트보다 커졌으면 알람
        if (addPoint > 0)
        {
            PublicSoundMethod.PlayClickReceive();
            PublicUIMethod.SetPopUpAlramMsg(string.Format("{0} : +{1}", NTextManager.Instance.GetText("UI_SEASON_PASS_POINT"), addPoint));
        }

        var passSchedule = GetPassSchedule((int)passKind);
        if (resultTier > curTier && passSchedule != null)
        {
            var passInfo = TableBattlePass_Pass.Instance.GetPass((int)passKind);
            if (passInfo != null)
            {
                switch (passInfo.PassUIType)
                {
                    case ePassUIType.BATTLE:
                    {
                        var battlePassDlg = UIFormManager.Instance.FindUIForm<PassMainDlg>();
                        if (null != battlePassDlg)
                            battlePassDlg.PlayTierUpAni();
                    }
                        break;
                    case ePassUIType.PRIME:
                    {
                        var primePassDlg = UIFormManager.Instance.FindUIForm<PassPrimeDlg>();
                        if (null != primePassDlg)
                            primePassDlg.PlayTierUpAni();
                    }
                        break;
                    case ePassUIType.HUNTING:
                    {
                        var huntingPassDlg = UIFormManager.Instance.FindUIForm<HuntingPassDlg>();
                        if (null != huntingPassDlg)
                            huntingPassDlg.PlayTierUpAni();
                    }
                        break;
                }
            }
        }

        if (m_User.ContainsKey(completeMission.Kind) == true)
            m_User[completeMission.Kind].Point = passPoint;

        // ui 갱신
        UIFormManager.Instance.RefreshUIForm<PassMainDlg>();
        UIFormManager.Instance.RefreshUIForm<PassPrimeDlg>();
        UIFormManager.Instance.RefreshUIForm<HuntingPassDlg>();

        PassMissionDlg dlg = UIFormManager.Instance.FindUIForm<PassMissionDlg>();
        if (null != dlg && dlg.Visible)
        {
            dlg.SetTextAndSprite();
            dlg.SetWeekTab();
            dlg.Refresh();
        }

        MainTopDlg MainDlg = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (null != MainDlg)
        {
            MainDlg.Update_EventGroup(MainTopDlg.eBTN_GROUP_TYPE.SHOP, true);
        }
    }

    public void RECV_GS_BATTLEPASS_TIER_REWARD_ACK(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_TIER_REWARD_ACK pkt)
    {
        // 내 티어 보상 상태 갱신
        for (int i = 0; i < pkt.TierLength; ++i)
        {
            var tierInfo = pkt.GetTier(i);
            if (m_Reward.ContainsKey(tierInfo.Kind))
            {
                if (m_Reward[tierInfo.Kind].Contains((tierInfo.Tier, (eBattlePassRewardType)tierInfo.RewardType)) == false)
                {
                    m_Reward[tierInfo.Kind].Add((tierInfo.Tier, (eBattlePassRewardType)tierInfo.RewardType));
                }
            }
            else
            {
                var rewardHash = new HashSet<(Int32, eBattlePassRewardType)>();
                rewardHash.Add((tierInfo.Tier, (eBattlePassRewardType)tierInfo.RewardType));
                m_Reward.Add(tierInfo.Kind, rewardHash);
            }
        }


        //보상이 1개 일때
        if(pkt.TierLength <= 1)
        {
            List<ObtainItemBase> ObtainItem = new List<ObtainItemBase>();
            // 아이템
            for (int i = 0; i < pkt.RewardItemLength; ++i)
            {
                var rewardItem = pkt.GetRewardItem(i);
                var itemInfo = BaseTable.TableItemInfo.Instance.Get((int)rewardItem.KIND);
                if (itemInfo == null)
                    continue;

                if (false == itemInfo.itemDirectUse)
                    Inventory.Instance.AddItemNum((int)rewardItem.KIND, rewardItem.Count);

                ObtainItem.Add(new ObtainItemBase((int)rewardItem.KIND, rewardItem.Count));
            }

            // 자원
            var addOil = pkt.AddAsset.Oil;
            var addIron = pkt.AddAsset.Iron;
            var addSilver = pkt.AddAsset.Silver;
            var addGold = pkt.AddAsset.Gold;

            User.Instance.assets.Set(pkt.CurAsset.Oil, pkt.CurAsset.Iron, pkt.CurAsset.Silver, pkt.CurAsset.Gold);

            if (0 < addOil)
                ObtainItem.Add(new ObtainItemBase((int)GameDefine.eRESOURCE_TYPE.eOIL, addOil));
            if (0 < addIron)
                ObtainItem.Add(new ObtainItemBase((int)GameDefine.eRESOURCE_TYPE.eIRON, addIron));
            if (0 < addSilver)
                ObtainItem.Add(new ObtainItemBase((int)GameDefine.eRESOURCE_TYPE.eSILVER, addSilver));
            if (0 < addGold)
                ObtainItem.Add(new ObtainItemBase((int)GameDefine.eRESOURCE_TYPE.eGOLD, addGold));

            // 젬
            long addGem = (pkt.AddVCGem + pkt.AddFreeGem);
            if (0 < addGem)
            {
                NdreamPayment.PaymentManager.Instance.CurGem += addGem;
                User.Instance.assets.Add(0, 0, 0, 0, addGem);
                ObtainItem.Add(new ObtainItemBase(9500, addGem));
            }

            // 쿠폰
            for (int i = 0; i < pkt.CouponsLength; ++i)
            {
                var coupon = pkt.GetCoupons(i);

                CouponManager.Instance.AddCoupon(new Coupon(coupon.UniqueID, coupon.IsActive, coupon.ItemKind, coupon.BeginTime, coupon.EndTime, coupon.DiscountRate,
                    TKString.BytesString(coupon.GetProductKindsBytes().GetValueOrDefault())));
                ObtainItem.Add(new ObtainItemBase(coupon.ItemKind, 1));
            }


            if(ObtainItem.Count > 0)
                ObtainItem_ControllerEx.EnQueueEvent(ObtainItem);
        }
        //보상이 2개 이상일 때 (모두 받기)
        else
        {
            List<Item> EventItem = new List<Item>();

            for (int i = 0; i < pkt.RewardItemLength; ++i)
            {
                var rewardItem = pkt.GetRewardItem(i);
                var itemInfo = BaseTable.TableItemInfo.Instance.Get((int)rewardItem.KIND);
                if (itemInfo == null)
                    continue;

                if (false == itemInfo.itemDirectUse)
                    Inventory.Instance.AddItemNum((int)rewardItem.KIND, rewardItem.Count);

                EventItem.Add(new Item((int)rewardItem.KIND, rewardItem.Count));
            }

            // 자원
            var addOil = pkt.AddAsset.Oil;
            var addIron = pkt.AddAsset.Iron;
            var addSilver = pkt.AddAsset.Silver;
            var addGold = pkt.AddAsset.Gold;

            User.Instance.assets.Set(pkt.CurAsset.Oil, pkt.CurAsset.Iron, pkt.CurAsset.Silver, pkt.CurAsset.Gold);

            if (0 < addOil)
                EventItem.Add(new Item((int)GameDefine.eRESOURCE_TYPE.eOIL, addOil));
            if (0 < addIron)
                EventItem.Add(new Item((int)GameDefine.eRESOURCE_TYPE.eIRON, addIron));
            if (0 < addSilver)
                EventItem.Add(new Item((int)GameDefine.eRESOURCE_TYPE.eSILVER, addSilver));
            if (0 < addGold)
                EventItem.Add(new Item((int)GameDefine.eRESOURCE_TYPE.eGOLD, addGold));

            // 젬
            long addGem = (pkt.AddVCGem + pkt.AddFreeGem);
            if (0 < addGem)
            {
                NdreamPayment.PaymentManager.Instance.CurGem += addGem;
                User.Instance.assets.Add(0, 0, 0, 0, addGem);
                EventItem.Add(new Item(9500, addGem));
            }

            // 쿠폰
            for (int i = 0; i < pkt.CouponsLength; ++i)
            {
                var coupon = pkt.GetCoupons(i);

                CouponManager.Instance.AddCoupon(new Coupon(coupon.UniqueID, coupon.IsActive, coupon.ItemKind, coupon.BeginTime, coupon.EndTime, coupon.DiscountRate,
                    TKString.BytesString(coupon.GetProductKindsBytes().GetValueOrDefault())));
                EventItem.Add(new Item(coupon.ItemKind, 1));
            }

            if (EventItem.Count > 0)
                EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(new EVENT.GetItemPackageDirection(EventItem, 0, 0, 1));
        }

        // 여기서 필요한 dlg 갱신처리 해줘야한다.
        // ui 갱신
        PassMainDlg dlg = UIFormManager.Instance.FindUIForm<PassMainDlg>();
        if (null != dlg && dlg.Visible)
        {
            dlg.ReceiveRewardAnimGrid();
            dlg.Refresh();
        }

        PassPrimeDlg primeDlg = UIFormManager.Instance.FindUIForm<PassPrimeDlg>();
        if(primeDlg != null && primeDlg.Visible)
        {
            primeDlg.ReceiveRewardAnimGrid();
            primeDlg.Refresh();
        }

        HuntingPassDlg huntingDlg = UIFormManager.Instance.FindUIForm<HuntingPassDlg>();
        if (huntingDlg != null && huntingDlg.Visible)
        {
            huntingDlg.ReceiveRewardAnimGrid();
            huntingDlg.Refresh();
        }

        MainTopDlg MainDlg = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (null != MainDlg)
        {
            MainDlg.Update_EventGroup(MainTopDlg.eBTN_GROUP_TYPE.SHOP, true);
        }

    }

    public void RECV_GS_BATTLEPASS_MISSION_NFY(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_MISSION_NFY pkt)
    {
        Dictionary<int /*passKind*/, HashSet<(eBattlePassMissionType, int)>> dictRefrehMissionScheduleType = new();

        for (var i = 0; i < pkt.MissionLength; ++i)
        {
            var nfyMissionInfo = pkt.GetMission(i);

            var missionInfo = TableBattlePass_Mission.Instance.GetMission(nfyMissionInfo.Kind, nfyMissionInfo.MissionKind);
            if(missionInfo == null)
                continue;

            if (m_Mission.ContainsKey(nfyMissionInfo.Kind))
            {
                var passMissions = m_Mission[nfyMissionInfo.Kind];
                if (passMissions.ContainsKey(nfyMissionInfo.MissionKind))
                {
                    //혹시 온게 만료 된 미션이면 미션쪽에서 삭제.
                    if (nfyMissionInfo.State == (int)eBattlePassMissionState.EXPIRE)
                    {
                        m_Mission[nfyMissionInfo.Kind].Remove(nfyMissionInfo.MissionKind);
                        continue;
                    }

                    var targetMission = m_Mission[nfyMissionInfo.Kind][nfyMissionInfo.MissionKind];

                    //혹시 완료면 노티
                    if(nfyMissionInfo.State == (int)eBattlePassMissionState.COMPLETE && targetMission.MissionState == eBattlePassMissionState.PROCESSING)
                    {
                        var scheduleInfo = GetPassSchedule(nfyMissionInfo.Kind);
                        var passInfo = TableBattlePass_Pass.Instance.GetPass(nfyMissionInfo.Kind);
                        if (scheduleInfo != null && passInfo != null)
                        {
                            switch (passInfo.PassUIType)
                            {
                                case ePassUIType.BATTLE:
                                    EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(new EVENT.PopUpSetBuffDlgDirection(NTextManager.Instance.GetText(missionInfo.MissionTitle), NTextManager.Instance.GetText("UI_SEASON_PASS_MISSION_COMPLETE")));
                                    break;
                                case ePassUIType.PRIME:
                                case ePassUIType.HUNTING:
                                {
                                    string textKey = string.Format("UI_PRIME_PASS{0:D2}_TEXT_28", nfyMissionInfo.Kind);
                                    EVENT.EventDirectionQueueManager.Instance.Add_Field_Territory_WaitEvent(new EVENT.PopUpSetBuffDlgDirection(NTextManager.Instance.GetText(missionInfo.MissionTitle), NTextManager.Instance.GetText(textKey)));
                                }
                                    break;
                            }

                        }
                    }

                    // 진행 중에서 완료로 변경된 미션 스케쥴 타입만 캐싱
                    if (targetMission.MissionState == eBattlePassMissionState.PROCESSING &&
                        (eBattlePassMissionState) nfyMissionInfo.State == eBattlePassMissionState.COMPLETE)
                    {
                        if (!dictRefrehMissionScheduleType.ContainsKey(nfyMissionInfo.Kind))
                            dictRefrehMissionScheduleType[nfyMissionInfo.Kind] = new HashSet<(eBattlePassMissionType, int)>();

                        dictRefrehMissionScheduleType[nfyMissionInfo.Kind].Add((missionInfo.MissionScheduleType, missionInfo.MissionScheduleValue));
                    }

                    targetMission.PassKind = nfyMissionInfo.Kind;
                    targetMission.Missionkind = nfyMissionInfo.MissionKind;
                    targetMission.CurValue = nfyMissionInfo.CurValue;
                    targetMission.MissionState = (eBattlePassMissionState)nfyMissionInfo.State;
                }
                else
                {
                    if (nfyMissionInfo.State == (int)eBattlePassMissionState.EXPIRE)
                        continue;

                    var missionData = new BattlePassMission();
                    missionData.PassKind = nfyMissionInfo.Kind;
                    missionData.Missionkind = nfyMissionInfo.MissionKind;
                    missionData.CurValue = nfyMissionInfo.CurValue;
                    missionData.MissionState = (eBattlePassMissionState)nfyMissionInfo.State;

                    passMissions.Add(nfyMissionInfo.MissionKind, missionData);
                }
            }
            else
            {
                if (nfyMissionInfo.State == (int)eBattlePassMissionState.EXPIRE)
                    continue;

                Dictionary<Int64, BattlePassMission> dicMissionData = new Dictionary<long, BattlePassMission>();

                var missionData = new BattlePassMission();

                missionData.PassKind = nfyMissionInfo.Kind;
                missionData.Missionkind = nfyMissionInfo.MissionKind;
                missionData.CurValue = nfyMissionInfo.CurValue;
                missionData.MissionState = (eBattlePassMissionState)nfyMissionInfo.State;

                dicMissionData.Add(nfyMissionInfo.MissionKind, missionData);

                m_Mission.Add(nfyMissionInfo.Kind, dicMissionData);
            }


        }

        // 여기서 필요한 dlg 갱신처리 해줘야한다.

        PassMissionDlg dlg = UIFormManager.Instance.FindUIForm<PassMissionDlg>();
        if (null != dlg && dlg.Visible)
        {
            dlg.SetTextAndSprite();
            dlg.SetWeekTab();
            dlg.SetMissionGrid();
        }


        MainTopDlg MainDlg = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (null != MainDlg)
        {
            MainDlg.Update_EventGroup(MainTopDlg.eBTN_GROUP_TYPE.SHOP, true);
        }

        // 여기에 있는 미션 타입들은 다시 레드닷 처리를 해줄 것
        foreach (var (passKind, setMissionScheduleType) in dictRefrehMissionScheduleType)
        {
            foreach (var (missionScheduleType, missionScheduelValue) in setMissionScheduleType)
            {
                SetMissionTypeIgnore(passKind, missionScheduleType, missionScheduelValue, false);
            }
        }
    }

    public void RECV_GS_BATTLEPASS_TIER_PURCHASE_ACK(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_TIER_PURCHASE_ACK pkt)
    {
        var passKind = pkt.PassKind;
        var passRound = pkt.Round;
        var passRewardGroup = pkt.RewardGroup;
        var passPoint = pkt.Point;
        var resultGem = pkt.Gem;

        if (!m_User.ContainsKey(passKind))
            return;

        var passSchedule = GetPassSchedule(passKind);
        if (passSchedule == null)
            return;

        var passInfo = TableBattlePass_Pass.Instance.GetPass(passKind);
        if (passInfo == null)
            return;

        var userBattlePass = m_User[passKind];
        userBattlePass.Point = passPoint;

        switch (passInfo.PassUIType)
        {
            case ePassUIType.BATTLE:
            {
                var dlg = UIFormManager.Instance.FindUIForm<PassMainDlg>();
                if (null != dlg)
                    dlg.PlayTierUpAni();
            }
                break;
            case ePassUIType.PRIME:
            {
                var dlg = UIFormManager.Instance.FindUIForm<PassPrimeDlg>();
                if (null != dlg)
                    dlg.PlayTierUpAni();
            }
                break;
            case ePassUIType.HUNTING:
            {
                var dlg = UIFormManager.Instance.FindUIForm<HuntingPassDlg>();
                if (null != dlg)
                    dlg.PlayTierUpAni();
            }
                break;
        }


        PaymentManager.Instance.CurGem = resultGem;

        UIFormManager.Instance.RefreshUIForm<PassMainDlg>();
        UIFormManager.Instance.RefreshUIForm<PurchaseTierDlg>();
        UIFormManager.Instance.RefreshUIForm<PassPrimeDlg>();
        UIFormManager.Instance.RefreshUIForm<HuntingPassDlg>();
        //GameEvent.DISPATCH_DATA<TerritoryBuilding>.CACHED.Set(null).Dispatch();

        MainTopDlg MainDlg = UIFormManager.Instance.FindUIForm<MainTopDlg>();
        if (null != MainDlg)
        {
            MainDlg.Update_EventGroup(MainTopDlg.eBTN_GROUP_TYPE.SHOP, true);
        }
    }

    public void RECV_GS_BATTLEPASS_PASS_PURCHASE_ACK(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_PASS_PURCHASE_ACK pkt)
    {
        var result = pkt.Result;
        var needRequestToWas = pkt.RequestToWas;
        var passKind = pkt.PassKind;
        var passRound = pkt.PassRound;
        var passPoint = pkt.Point;
        var resultPasType = pkt.PassType;
        var resultGem = pkt.Gem;
        var curTier = GetCurTier((int)passKind);
        var resultTier = passPoint / BattlePassManager.CONST_ONE_TIER;

        if (result != (int) (eRESULT.R_OK))
        {
            if (result == (int)eRESULT.R_FAIL_DISCORD_ASSET)
            {
                if (resultGem >= 0)
                    PaymentManager.Instance.CurGem = resultGem;
            }

            return;
        }

        // was 로 직접 조요청
        if (needRequestToWas == true)
        {
            if (!m_User.ContainsKey(passKind))
                return;

            var PassInfo = TableBattlePass_Pass.Instance.GetPass(passKind);
            if (PassInfo == null)
                return;

            if (PassInfo.PackageKind != pkt.RequestProductKind)
                return;

            PackageShopData pkgShopData = PaymentManager.Instance.PackageShop.GetbyKind(pkt.RequestProductKind);
            if (pkgShopData == null)
                return;

            pkgShopData.GetPrice(out string disRate, out string disPrice, out string oriPrice);

            if (PublicUIMethod.GetChekedHaveGem(pkgShopData.IsUseGem, disPrice, PurchaseShopType.GEM_SHOP) == true)
            {
                if (pkgShopData.ShopPoint > 0)
                {
                    if (PaymentManager.Instance.CurShopPoint < pkgShopData.ShopPoint)
                    {
                        MessageBoxDlg messageBoxDlg = UIFormManager.Instance.GetUIForm<MessageBoxDlg>();
                        if (messageBoxDlg != null)
                        {
                            messageBoxDlg.Set(NTextManager.Instance.GetText("UI_COMMON_NOTICE"),
                                NTextManager.Instance.GetText("UI_POINTSHOP_MESSAGE_SHORT"),
                                NTextManager.Instance.GetText("UI_COMMON_CONFIRM_YES"));
                            messageBoxDlg.Open();
                            return;
                        }
                    }
                }

                UIFormManager.Instance.ShowLoadingSpinWait();
                PaymentManager.Instance.CheckPurchase(pkgShopData.PackageKind, "BattlePassDlg");
            }
        }
        // 여기로 왔다면 프로시저에서 완료처리까지 하고 모두 구매된 상황이다.
        else
        {
            if (!m_User.ContainsKey(passKind))
                return;

            if(resultTier > curTier)
            {
                var passSchedule = GetPassSchedule((int)passKind);
                if(passSchedule != null)
                {
                    var dlg = UIFormManager.Instance.FindUIForm<PassMainDlg>();
                    if (null != dlg)
                        dlg.PlayTierUpAni();
                }
            }



            var userBattlePass = m_User[passKind];
            if (userBattlePass.PassType == (eBattlePassType)resultPasType)
                return;

            userBattlePass.Point = passPoint;
            userBattlePass.PassType = (eBattlePassType) resultPasType;


            if (resultGem >= 0)
                PaymentManager.Instance.CurGem = resultGem;

            UIFormManager.Instance.RefreshUIForm<PassMainDlg>();
            UIFormManager.Instance.RefreshUIForm<PassMissionDlg>();
            UIFormManager.Instance.CloseUIForm<BattlePassDlg>();

            MainTopDlg MainDlg = UIFormManager.Instance.FindUIForm<MainTopDlg>();
            if (null != MainDlg)
            {
                MainDlg.Update_EventGroup(MainTopDlg.eBTN_GROUP_TYPE.SHOP, true);
            }
        }

    }

    public void RECV_GS_BATTLEPASS_PASS_PURCHASE_NFY(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_PASS_PURCHASE_NFY pkt)
    {
        var result = pkt.Result;
        var passKind = pkt.PassKind;
        var passRound = pkt.PassRound;
        var passPoint = pkt.Point;
        var resultPasType = pkt.PassType;

        // 패스를 구매했을 때 메세지
        //string strPackage = ack.PassType == (int)SeasonPassManager.ePASS_TYPE.SILVER ? "UI_SEASON_PASS_GOLD_PASS_1" : "UI_SEASON_PASS_GOLD_PASS_2";
        //PublicUIMethod.SetPopUpAlramMsg(string.Format(NTextManager.Instance.GetText("UI_SEASON_PASS_GOLD_PASS_BUY_CONFIRM"), string.Format(NTextManager.Instance.GetText(strPackage), SeasonPassManager.Instance.GetCurSeason().ToString())));

        if (!m_User.ContainsKey(passKind))
            return;

        var userBattlePass = m_User[passKind];
        if (userBattlePass.PassType == (eBattlePassType)resultPasType)
            return;

        if (userBattlePass.Round != passRound)
            return;

        userBattlePass.Point = passPoint;
        userBattlePass.PassType = (eBattlePassType)resultPasType;


        UIFormManager.Instance.RefreshUIForm<PassMainDlg>();
        UIFormManager.Instance.RefreshUIForm<PassMissionDlg>();
        UIFormManager.Instance.RefreshUIForm<PassPrimeDlg>();
        UIFormManager.Instance.RefreshUIForm<HuntingPassDlg>();
        UIFormManager.Instance.CloseUIForm<BattlePassDlg>();

        //Action toastCallback = null;

        //NrSeasonPass_Pass passInfo = TableSeasonPass_Pass.Instance.Get(SeasonPassManager.Instance.GetCurSeason(), (int)SeasonPassManager.ePASS_TYPE.GOLD);
        //if (null == passInfo && passInfo.listGiveItem.Count > 0)
        //{
        //    int toastItemKind = passInfo.listGiveItem[0].kind;
        //    long toastItemCount = passInfo.listGiveItem[0].num;

        //    var toastItemInfo = TableItemInfo.Instance.Get(toastItemKind);
        //    if (toastItemInfo != null && toastItemCount > 0)
        //    {
        //        toastCallback = () =>
        //        {
        //            PublicUIMethod.AddPopUpAlramMsg(string.Format(NTextManager.Instance.GetText("UI_SEASON_PASS_BUY_GOLD_01"),
        //                NTextManager.Instance.GetText(toastItemInfo.itemName),
        //                PublicUIMethod.ThousandSeparateString(toastItemCount)));
        //        };
        //    }
        //}

        // 티어업 연출
        //if (iPrevTier != SeasonPassManager.Instance.m_User.m_iTier)
        //{
        //    var dlg = UIFormManager.Instance.FindUIForm<PassMainDlg>();
        //    if (null != dlg)
        //        dlg.PlayTierUpAnim();
        //}

        //toastCallback?.Invoke();

    }

    public void RECV_GS_BATTLEPASS_POINT_NFY(PROTOCOL.FLATBUFFERS.GS_BATTLEPASS_POINT_NFY pkt)
    {
        var PassKind = pkt.PassKind;
        var PassRound = pkt.PassRound;
        var PassPoint = pkt.PassPoint;
        var PassType = pkt.PassType;

        if (m_User.ContainsKey(PassKind) == false)
            return;

        var userBattlePass = m_User[PassKind];

        if (userBattlePass.Round != PassRound)
            return;

        if (userBattlePass.PassType != (eBattlePassType) PassType)
            return;

        var addPoint = PassPoint - userBattlePass.Point;
        //기존 포인트보다 커졌으면 알람
        if (addPoint > 0)
        {
            PublicSoundMethod.PlayClickReceive();
            PublicUIMethod.SetPopUpAlramMsg(string.Format("{0} : +{1}", NTextManager.Instance.GetText("UI_SEASON_PASS_POINT"), addPoint));
        }

        userBattlePass.Point = PassPoint;

        // 아이템 조각 사용했을 때만 들어오는 로직이니까 클라 갱신처리해주자
        if (pkt.UseItem != null)
        {
            var  usingItemInfo = TableItemInfo.Instance.Get(pkt.UseItem.ItemKind);
            // 사용 아이템 갯수 차감
            Inventory.Instance.AddItemNum(pkt.UseItem.ItemKind, -pkt.UseItem.ItemNum);
            Inventory.Instance.RemovePeriodItem(pkt.UseItem.ItemKind);

            ITEM_INFO_UPDATE.CACHED.Reset().Set(pkt.UseItem.ItemKind).Dispatch();
        }

    }

    public bool CheckPassBuffStat(int passKind, BuffContents.Defines.eBUFF_STAT_KIND _buffStatKind)
    {
        bool hasBuffStat = false;

        var passInfo = TableBattlePass_Pass.Instance.GetPass(passKind);
        if (passInfo == null)
            return hasBuffStat;

        foreach (var buffItemKind in passInfo.BuffItems)
        {
            var pItemInfo = TableItemInfo.Instance.Get(buffItemKind);
            if (pItemInfo == null)
                continue;

            var buffInfo = TableBuffElemInfo.Instance.Get(pItemInfo.itemvalue1);
            if (buffInfo == null)
                continue;

            if (buffInfo.ContainsKind((int)_buffStatKind))
            {
                hasBuffStat = true;
                break;
            }
        }

        return hasBuffStat;
    }
    public bool IsActiveBattlePass(DEFINE.GameDefine.eBattlePassScheduleType type, out int activePassKind)
    {
        activePassKind = 0;

        if (PublicMethod.IsBattleRoyalServer() || PublicMethod.IsServerWarsServer())
            return false;

        if (m_User == null)
            return false;

        foreach(var userPass in m_User)
        {
            var passInfo = TableBattlePass_Schedule.Instance.GetPassSchedule(userPass.Key);
            if (passInfo == null)
                continue;

            if (type == passInfo.PassType)
            {
                activePassKind = userPass.Key;
                return true;
            }
        }

        return false;
    }

    public bool IsActiveBattlePass(int passKind)
    {
        if (!m_User.ContainsKey(passKind))
            return false;

        var passInfo = TableBattlePass_Pass.Instance.GetPass(passKind);
        if (passInfo == null)
            return false;

        return true;
    }

    public BattlePassUser GetBattlePassUser(Int32 kind)
    {
        BattlePassUser reuslt = null;
        if (PublicMethod.IsBattleRoyalServer() || PublicMethod.IsServerWarsServer())
            return null;

        if (m_User.Count <= 0)
            return null;

        m_User.TryGetValue(kind, out reuslt);

        return reuslt;
    }

    public Int32 GetNeedPoint(Int32 kind, Int32 curTier)
    {
        if (m_User.Count <= 0)
            return 0;

        Int32 needPoint = 0;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if(userData != null)
        {
            var pointInfo = TableBattlePass_Point.Instance.GetPointInfo(userData.PassKind, userData.RewardGroup, curTier);
            needPoint = pointInfo == null ? 0 : pointInfo.NeedPoint; ;
        }

        return needPoint;
    }

    public Int32 GetMaxTier(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);
        if(userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(userData.PassKind, userData.RewardGroup);

            if(pointInfos != null)
            {
                result = pointInfos.Count;
            }
        }

        return result;
    }
    public Int32 GetPurchaseAbleMaxTier(Int32 kind)
    {
        Int32 result = 0;

        var calcTier = 0;
        var curTier = GetCurTier(kind);
        var nextTier = curTier + 1;
        var maxTier = GetMaxTier(kind);

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if (userData != null)
        {
            if (nextTier > maxTier)
                nextTier = maxTier;

            var pointInfo = TableBattlePass_Point.Instance.GetPointInfo(userData.PassKind, userData.RewardGroup, nextTier);
            Int32 premium = 0;
            if (pointInfo != null)
                premium = pointInfo.PremiumReward;

            if (premium < (int)eBattlePassPremiumRewardType.PREMIUM_SHOW)
                premium = (int)eBattlePassPremiumRewardType.PREMIUM_SHOW;

            var pointinfos = TableBattlePass_Point.Instance.GetPointInfos(userData.PassKind, userData.RewardGroup);
            if (pointinfos == null)
                return result;

            foreach(var (keyTier, valuePoint) in pointinfos)
            {
                if (valuePoint.PremiumReward > premium)
                    continue;

                if (keyTier > calcTier)
                    calcTier = keyTier;
            }
        }

        return result = calcTier == 0 ? maxTier : calcTier;
    }

    public Dictionary<int, Int64> GetReward(Int32 kind, Int32 curTier, Int32 targetTier)
    {
        Dictionary<int, Int64> result = new Dictionary<int, Int64>();

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        var curType = GetPassType(kind);

        List<Item> itemList = new List<Item>();
        if(userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(userData.PassKind, userData.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach(var (keyTier, valuePoint) in pointInfos)
            {
                if (valuePoint == null)
                    continue;

                if(curTier < keyTier && keyTier <= targetTier)
                {
                    for(int i = 0; i < valuePoint.FreeReward.Count; ++i)
                    {
                        if (result.ContainsKey(valuePoint.FreeReward[i].kind))
                            result[valuePoint.FreeReward[i].kind] += valuePoint.FreeReward[i].num;
                        else
                            result.Add(valuePoint.FreeReward[i].kind, valuePoint.FreeReward[i].num);
                    }

                    if(curType >= eBattlePassType.PAID)
                    {
                        for (int i = 0; i < valuePoint.PaidReward.Count; ++i)
                        {
                            if (result.ContainsKey(valuePoint.PaidReward[i].kind))
                                result[valuePoint.PaidReward[i].kind] += valuePoint.PaidReward[i].num;
                            else
                                result.Add(valuePoint.PaidReward[i].kind, valuePoint.PaidReward[i].num);
                        }
                    }
                }
            }
        }

        return result;
    }

    public Int32 GetTierUpNeedGem(Int32 kind, Int32 curTier, Int32 targetTier)
    {
        Int32 result = 0;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if(userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(userData.PassKind, userData.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach(var (keyTier, valuePoint) in pointInfos)
            {
                if (valuePoint == null)
                    continue;

                if (curTier < keyTier && keyTier <= targetTier)
                    result += valuePoint.NeedGem;
            }
        }

        return result;
    }

    public bool IsMaxTier(Int32 kind)
    {
        bool result = false;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if(userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(userData.PassKind, userData.RewardGroup);

            if (pointInfos != null)
            {
                var curTier = GetCurTier(userData.PassKind);
                result = pointInfos.Count <= curTier;
            }
        }

        return result;
    }

    public Dictionary<Int32, NrBattlePass_Point> GetPointInfos(Int32 kind, Int32 rewardGroup)
    {
        return TableBattlePass_Point.Instance.GetPointInfos(kind, rewardGroup);
    }

    public NrBattlePass_Point GetPointInfo(Int32 kind, Int32 rewardGroup, Int32 Tier)
    {
        return TableBattlePass_Point.Instance.GetPointInfo(kind,rewardGroup,Tier);
    }

    public bool GetRewarded(Int32 kind, Int32 tier, eBattlePassRewardType type)
    {
        bool result = false;
        HashSet<(Int32, eBattlePassRewardType)> userReward = null;

        BattlePassUser userInfo = null;
        m_User.TryGetValue(kind, out userInfo);
        if (userInfo == null)
            return result;

        m_Reward.TryGetValue(kind, out userReward);
        if(userReward != null)
        {
            var pointInfo = TableBattlePass_Point.Instance.GetPointInfo(kind, userInfo.RewardGroup, tier);
            if (pointInfo == null)
                return result;

            //애초에 Ndt에 없으면 그냥 보상 받을 수 없다고 보내줘야한다.
            if (type == eBattlePassRewardType.FREE && pointInfo.FreeReward.Count <= 0)
                result = true;

            if (type == eBattlePassRewardType.PAID && pointInfo.PaidReward.Count <= 0)
                result = true;

            //유저가 받은 보상 중에 있다~
            if (userReward.Contains((tier, type)) == true)
                result = true;
        }

        return result;
    }
    public eBattlePassType GetPassType(Int32 kind)
    {
        eBattlePassType result = eBattlePassType.NONE;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if (userPass != null)
            result = userPass.PassType;

        return result;
    }
    public bool GetRewardAble(Int32 kind, Int32 tier, eBattlePassRewardType type)
    {
        bool result = false;

        //해당 티어에 대한 보상 이미 받았으면 그냥 바로 return
        var rewarded = GetRewarded(kind, tier, type);
        if (rewarded == true)
            return result;

        //받지 않았고 해당 티어에 필요한 포인트보다 높으면 보상 받을 수 있다.
        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if( userPass != null)
        {
            var myPoint = userPass.Point;
            var totalNeedPoint = TableBattlePass_Point.Instance.GetTotalNeedPoint(userPass.PassKind, userPass.RewardGroup, tier);
            if (myPoint >= totalNeedPoint)
                result = true;
        }

        return result;
    }
    public Int32 GetRewardAbleTotalCount(Int32 kind, Int32 curTier)
    {
        Int32 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);



        if (userPass != null)
        {
            var myPoint = userPass.Point;
            Dictionary<Int32, NrBattlePass_Point>  pointInfos = TableBattlePass_Point.Instance.GetPointInfos(userPass.PassKind, userPass.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach (var (keyTier, valuePoint) in pointInfos)
            {
                if(keyTier <= curTier)
                {
                    if (GetRewarded(userPass.PassKind, keyTier, eBattlePassRewardType.FREE) == false)
                        ++result;

                    if (userPass.PassType == eBattlePassType.PAID && GetRewarded(userPass.PassKind, keyTier, eBattlePassRewardType.PAID) == false)
                        ++result;
                }
            }
        }

        return result;
    }

    public Int64 GetUserPoint(Int32 kind)
    {
        Int64 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if (userPass != null)
            result = userPass.Point;

        return result;
    }

    public Int32 GetCurTier(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if (userPass != null)
            result = ((Int32)(userPass.Point / BattlePassManager.CONST_ONE_TIER));

        return result;
    }


    public NrBattlePass_Point GetNextPremium(Int32 kind, Int32 curTier)
    {
        NrBattlePass_Point result = null;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if(userPass != null)
            result = TableBattlePass_Point.Instance.GetNextPremium(userPass.PassKind, userPass.RewardGroup, curTier);

        return result;
    
    }
    
    public Int32 GetPremiumMinTier(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if(userPass != null)
        {
            var pointInfos = GetPointInfos(userPass.PassKind, userPass.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach(var (tier, point) in pointInfos)
            {
                if (point.PremiumReward == (int)eBattlePassPremiumRewardType.PLUS_TIER)
                {
                    result = tier;
                    break;
                }
            }
        }

        return result;
    }

    public Int32 GetPremiumRewardTier(Int32 kind, eBattlePassPremiumRewardType type)
    {
        Int32 result = 0;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);
        var curTier = GetCurTier(kind);

        if(userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(kind, userData.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach(var (tier, point) in pointInfos)
            {
                if (GetRewarded(kind, tier, eBattlePassRewardType.PAID) == true)
                    continue;

                if (point.PremiumReward == (int)type && curTier >= tier)
                {
                    result = tier;
                    return result;
                }
            }
        }

        return result;
    }

    public Int32 GetCurRewardAbleTier(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);
        var curTier = GetCurTier(kind);
        
        if (userData != null)
        {
            var pointInfos = TableBattlePass_Point.Instance.GetPointInfos(kind, userData.RewardGroup);
            if (pointInfos == null)
                return result;

            foreach (var (tier, point) in pointInfos)
            {
                if(userData.PassType == eBattlePassType.PAID)
                {
                    if (GetRewarded(kind, tier, eBattlePassRewardType.PAID) == true && GetRewarded(kind, tier, eBattlePassRewardType.FREE) == true)
                        continue;
                }
                else
                {
                    if(GetRewarded(kind, tier, eBattlePassRewardType.FREE) == true)
                        continue;
                }

                result = tier;
                return result;
            }
        }

        return result;
    }

    public Int32 GetPremiumRewardAbleCount(Int32 kind, eBattlePassPremiumRewardType type)
    {
        Int32 result = 0;

        var curTier = GetCurTier(kind);

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if(userData != null)
        {
            var pointInfos = GetPointInfos(kind, userData.RewardGroup);
            if(pointInfos != null)
            {
                foreach(var (tier, point) in pointInfos)
                {
                    if (tier > curTier || point.PremiumReward != (int)type)
                        continue;

                    if (GetRewarded(kind, tier, eBattlePassRewardType.PAID) == false)
                        ++result;
                }
            }
        }

        return result;
    }

    public Int32 GetPremiumRewardLastTier(Int32 kind)
    {
        Int32 result = 0;

        var curTier = GetCurTier(kind);

        BattlePassUser userData = null;
        m_User.TryGetValue(kind, out userData);

        if (userData != null)
        {
            var pointInfos = GetPointInfos(kind, userData.RewardGroup);
            if (pointInfos != null)
            {
                foreach (var (tier, point) in pointInfos)
                {
                    if (point.PremiumReward == (int)eBattlePassPremiumRewardType.PLUS_TIER && result < tier)
                        result = tier;
                }
            }
        }

        return result;
    }



    public List<BattlePassMission> GetMissions(Int32 kind, DEFINE.GameDefine.eBattlePassMissionType tyoe)
    {
        List<BattlePassMission> result = new List<BattlePassMission>();

        Dictionary<Int64, BattlePassMission> passMissions = null;
        m_Mission.TryGetValue(kind, out passMissions);

        if(passMissions != null)
        {
            foreach(var (keyMissionKind, valueMission) in passMissions)
            {
                if (valueMission.MissionState == eBattlePassMissionState.EXPIRE)
                    continue;

                var mission = TableBattlePass_Mission.Instance.GetMission(kind, keyMissionKind);
                if (mission == null)
                    continue;

                if (mission.MissionScheduleType == tyoe)
                    result.Add(valueMission);
            }
        }

        return result;
    }

    public NrBattlePass_Mission GetMission(Int32 kind, Int64 missionKind)
    {
        return TableBattlePass_Mission.Instance.GetMission(kind, missionKind);
    }

    public Int32 GetToTalRewardAbleCount(Int32 kind, Int32 curTier)
    {
        return GetRewardAbleMissionCount(kind) + GetRewardAbleTotalCount(kind, curTier);
    }

    public Int32 GetRewardAbleMissionCount(Int32 passKind, DEFINE.GameDefine.eBattlePassMissionType type = GameDefine.eBattlePassMissionType.NONE)
    {
        Int32 result = 0;

        m_Mission.TryGetValue(passKind, out var passMissions);
        if (passMissions == null)
            return result;

        foreach (var (keyMissionKind, valueMission) in passMissions)
        {
            var missionInfo = TableBattlePass_Mission.Instance.GetMission(passKind, keyMissionKind);
            if(missionInfo == null)
                continue;

            // 이 미션에 대해 ingore가 걸려있다면 레드값 검사할 필요가 읎다
            if (GetMissionIgnoreType(passKind, missionInfo.MissionScheduleType, missionInfo.MissionScheduleValue, missionInfo.isPaidMission))
                continue;

            if(type == GameDefine.eBattlePassMissionType.NONE)
            {
                if (valueMission.MissionState == eBattlePassMissionState.COMPLETE)
                    ++result;
            }
            else
            {
                var mission = BattlePassManager.Instance.GetMission(passKind, valueMission.Missionkind);
                if(mission != null)
                {
                    if (mission.MissionScheduleType == type && valueMission.MissionState == eBattlePassMissionState.COMPLETE)
                        ++result;
                }
            }
        }

        return result;
    }

    public Int32 GetPrimePassRewardAbleMissionCount(Int32 kind, DEFINE.GameDefine.eBattlePassMissionType type)
    {
        Int32 result = 0;

        Dictionary<Int64, BattlePassMission> passMissions = null;
        m_Mission.TryGetValue(kind, out passMissions);

        if (passMissions == null)
            return result;

        foreach (var (keyMissionKind, valueMission) in passMissions)
        {
            var missionInfo = TableBattlePass_Mission.Instance.GetMission(kind, keyMissionKind);
            if (missionInfo == null)
                continue;

            if (missionInfo.MissionScheduleType != type)
                continue;

            if (valueMission.MissionState == eBattlePassMissionState.COMPLETE)
                ++result;
        }

        return result;
    }

    public Int32 GetRewardedMissionCount(Int32 kind)
    {
        Int32 result = 0;

        Dictionary<Int64, BattlePassMission> passMissions = null;
        m_Mission.TryGetValue(kind, out passMissions);

        if (passMissions == null)
            return result;

        foreach (var (keyMissionKind, valueMission) in passMissions)
        {
            if (valueMission.MissionState == eBattlePassMissionState.REWARDED)
                ++result;
        }

        return result;
    }

    public Int32 GetMaxWeek(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if(userPass != null)
        {
            result = Mathf.CeilToInt((float)(userPass.EndTime - userPass.StartTime) / (float)WEEK_DURATION);
        }

        return result;
    }

    public Int32 GetCurWeek(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        long curTime = PublicMethod.GetDueDay_UTC(PublicMethod._serverUTCTimeValue);
        var maxWeek = GetMaxWeek(kind);
        if (userPass != null)
        {
            for (int i = 1; i <= maxWeek; ++i)
            {
                if(curTime < userPass.StartTime + (WEEK_DURATION * i))
                {
                    result = i;
                    break;
                }
            }
        }

        return result;
    }

    public Int64 GetNextTimeToWeeK(Int32 kind, Int32 week)
    {
        Int64 result = 0;

        BattlePassUser userPass = null;
        m_User.TryGetValue(kind, out userPass);

        if(userPass != null)
        {
            Int64 lCurTime = PublicMethod.GetDueDay_UTC(PublicMethod._serverUTCTimeValue);
            result = userPass.StartTime + (WEEK_DURATION * (week - 1)) - lCurTime;
        }

        return result;
    }

    public bool CheckMission(Int32 passKind, Int64 missionKind)
    {
        bool result = false;

        Dictionary<Int64, BattlePassMission> missionInfos = null;
        m_Mission.TryGetValue(passKind, out missionInfos);

        if(missionInfos != null)
        {
            BattlePassMission mission = null;
            missionInfos.TryGetValue(missionKind, out mission);

            result = (mission != null && mission.MissionState != eBattlePassMissionState.EXPIRE);
        }

        return result;
    }

    public BattlePassMission GetUserMission(Int32 passKind, Int64 missionKind)
    {
        BattlePassMission result = null;

        Dictionary<Int64, BattlePassMission> missionInfos = null;
        m_Mission.TryGetValue(passKind, out missionInfos);

        if (missionInfos != null)
        {
            BattlePassMission mission = null;
            missionInfos.TryGetValue(missionKind, out mission);

            result = (mission != null && mission.MissionState != eBattlePassMissionState.EXPIRE ) ? mission : null;
        }

        return result;
    }

    public NrBattlePass_Pass GetPass(Int32 passkind)
    {
        return TableBattlePass_Pass.Instance.GetPass(passkind);
    }

    public NrBattlePass_Schedule GetPassSchedule(Int32 passkind)
    {
        return TableBattlePass_Schedule.Instance.GetPassSchedule(passkind);
    }
    private const string NOTICE_KEY = "BattlePass{0}_{1}";
    public void SetNoticeCount(Int32 kind)
    {
        BattlePassUser userData;
        m_User.TryGetValue(kind, out userData);

        //Value 에 Round
        if(userData != null)
            PlayerPrefs.SetInt(string.Format(NOTICE_KEY, User.Instance.UID.ToString(), userData.PassKind), userData.Round);
    }
    public Int32 GetNoticeCount(Int32 kind)
    {
        Int32 result = 0;

        BattlePassUser userData;
        m_User.TryGetValue(kind, out userData);
        if (userData != null)
            result = PlayerPrefs.GetInt(string.Format(NOTICE_KEY, User.Instance.UID.ToString(), userData.PassKind), 0);

        return result;
    }

    public bool IsWeekMissionRewardAble(Int32 kind, Int32 week)
    {
        bool result = false;
        var missions = GetRewardAbleMission(kind);

        if (missions == null)
            return result;

        foreach(var mission in missions)
        {
            var missionInfo = TableBattlePass_Mission.Instance.GetMission(kind, mission.Missionkind);
            if (missionInfo == null)
                continue;

            if (missionInfo.MissionScheduleType == GameDefine.eBattlePassMissionType.WEEK && missionInfo.MissionScheduleValue == week)
            {
                result = true;
                break;
            }

        }
        return result;
    }
    public Int32 GetRewardAbleWeekly(Int32 kind, Int32 week)
    {
        Int32 result = 0;

        var missions = GetRewardAbleMission(kind);

        if (missions == null)
            return result;

        foreach(var mission in missions)
        {
            var missionInfo = TableBattlePass_Mission.Instance.GetMission(kind, mission.Missionkind);
            if (missionInfo == null)
                continue;

            if(missionInfo.MissionScheduleType == GameDefine.eBattlePassMissionType.WEEK && missionInfo.MissionScheduleValue == week)
            {
                if (mission.MissionState == eBattlePassMissionState.COMPLETE)
                    ++result;
            }
        }

        return result;
    }



    public List<BattlePassMission> GetRewardAbleMission(Int32 kind)
    {
        List<BattlePassMission> result = new List<BattlePassMission>();

        Dictionary<Int64, BattlePassMission> missionInfos = null;
        m_Mission.TryGetValue(kind, out missionInfos);

        if (missionInfos == null)
            return result;

        var curPassType = GetPassType(kind);
        foreach (var (missionkind, mission) in missionInfos)
        {
            var missionInfo = TableBattlePass_Mission.Instance.GetMission(kind, missionkind);
            if (missionInfo == null)
                continue;

            if (mission.MissionState == eBattlePassMissionState.COMPLETE)
                result.Add(mission);
        }

        return result;
    }

    public void OpenPass(int _passKind)
    {
        ePassUIType uiType = TableBattlePass_Pass.Instance.GetUIType(_passKind);
        if (uiType == ePassUIType.NONE)
            return;

        switch (uiType)
        {
            case ePassUIType.BATTLE:
                UIFormManager.Instance.OpenUIForm<PassMainDlg>(PassMainDlg.PARAM.CACHED.Set(_passKind));

                break;
            case ePassUIType.PRIME:
                UIFormManager.Instance.OpenUIForm<PassPrimeDlg>(PassPrimeDlg.PARAM.CACHED.Set(_passKind));
                break;
            case ePassUIType.HUNTING:
                UIFormManager.Instance.OpenUIForm<HuntingPassDlg>(HuntingPassDlg.PARAM.CACHED.Set(_passKind));
                break;
        }
    }



}


