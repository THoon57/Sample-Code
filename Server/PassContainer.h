#pragma once

class CUser;

class CBattlePassReward : public std::enable_shared_from_this<CBattlePassReward>
{
public:
	// PassKind, Round, Tier, RewardType
	using UpdateRewardInfos = std::set<std::tuple<INT64, INT32, INT32, BattlePass::eRewardType>>;
	
public:
	using SharedPtr = std::shared_ptr<CBattlePassReward>;
	using WeakPtr = std::weak_ptr<CBattlePassReward>;

public:
	INT32	m_PassKind = 0;
	INT32	m_Round = 0;
	INT32	m_Tier = 0;	
	BattlePass::eRewardType m_RewardType = BattlePass::eRewardType::NONE;
};

class CBattlePassPoint : public std::enable_shared_from_this<CBattlePassPoint>
{
public:
	// m_PassKind, m_Round, m_Point
	using UpdatePassInfo = std::tuple<INT32, INT32, INT64>;

	// m_PassKind, m_Round, m_PassType, m_Point
	using RollbackPassInfo = std::tuple<INT32, INT32, BattlePass::ePassType, INT64>;
	
public:
	using SharedPtr = std::shared_ptr<CBattlePassPoint>;
	using WeakPtr = std::weak_ptr<CBattlePassPoint>;

public:
	INT32	m_PassKind = 0;
	INT32	m_Round = 0;
	BattlePass::ePassType m_PassType = BattlePass::ePassType::FREE;
	INT64	m_curPoint = 0;
};

class CBattlePassMission : public std::enable_shared_from_this<CBattlePassMission>
{
public:
	// PassKind, Round, MissionKind, CurValue, State, RegistTime
	using UpdateMissionInfos = std::vector<std::tuple<INT64, INT32, INT64, INT32, BattlePass::ePassMissionState, INT64>>;
	// isNew, PassKind, Round, MissionKind, CurValue, State, RegistTime
	using RollbackMissionInfos = std::vector<std::tuple<BOOL, INT64, INT32, INT64, INT32, BattlePass::ePassMissionState, INT64>>;
	// PassKind, Round, MissionKind, CurValue, State, RegistTime
	using UpdateMissionInfo = std::tuple<INT64, INT32, INT64, INT32, BattlePass::ePassMissionState, INT64>;
	
public:
	using SharedPtr = std::shared_ptr<CBattlePassMission>;
	using WeakPtr = std::weak_ptr<CBattlePassMission>;

public:
	INT32	m_PassKind = 0;
	INT64	m_MissionKind = 0;
	INT64	m_ConditionKind = 0;
	INT64	m_CurValue = 0;
	INT32	m_Round = 0;
	BattlePass::ePassMissionState m_MissionState = BattlePass::ePassMissionState::NONE;
	INT64	m_RegistTime = 0; // 언제 등록된 녀석인지 알아야 이 데이터를 사용하던가 초기화하던가 결정할 수 있다.
};

class CBattlePass : public std::enable_shared_from_this<CBattlePass>
{
public:
	// UserID, PassKind, Round, RewardGroup, StartTime, EndTime, PassState
	using UpdateScheduleInfos = std::vector<std::tuple<INT64, INT32, INT32, INT32, INT64, INT64, BattlePass::ePassState>>;
	// isNew, PassKind, Round, RewardGroup, StartTime, EndTime, PassState
	using RollbackScheduleInfos = std::vector<std::tuple<BOOL, INT32, INT32, INT32, INT64, INT64, BattlePass::ePassState>>;
public:
	using SharedPtr = std::shared_ptr<CBattlePass>;
	using WeakPtr = std::weak_ptr<CBattlePass>;

public:
	INT32	m_PassKind = 0;
	INT64	m_StartTime = 0;
	INT64	m_EndTime = 0;
	INT32	m_RewardGroupID = 0;
	INT32	m_Round = 0;
	BattlePass::ePassState m_PassState = BattlePass::ePassState::NONE;

	CBattlePassPoint::SharedPtr m_spPoint;

	// Key : m_MissionKind
	std::unordered_map<INT64, CBattlePassMission::SharedPtr> m_MissionMap;

	std::set<INT32> m_setCheckWeek;
	
	// Key : Tier, RewardType
	std::unordered_map<pair<INT32,BattlePass::eRewardType>, CBattlePassReward::SharedPtr>	 m_RewardMap;

public:
	INT32 GetPassDay() const;
	INT32 GetPassWeek() const;

	BattlePass::ePassType GetPassType() const{ return m_spPoint == nullptr ? BattlePass::ePassType::FREE : m_spPoint->m_PassType; }
	INT64				  GetPassPoint() const { return m_spPoint == nullptr ? 0 : m_spPoint->m_curPoint; }

	void SetUnlockWeekMission();
};


class CBattlePassContainer
{
public:
	struct BattlePassDatum
	{
		typedef std::shared_ptr<BattlePassDatum>		SharedPtr;
		typedef std::weak_ptr<BattlePassDatum>		WeakPtr;

		CBattlePass::SharedPtr											pBattlePass = nullptr;
		std::unordered_map<INT32, BattlePassDatum::SharedPtr>::iterator	iter_BattlePass;
	};

	// key : PassKind
	typedef std::unordered_map<INT32, BattlePassDatum::SharedPtr>			BattlePassMap;

	// Key : ConditionKind
	typedef std::unordered_multimap<INT32, CBattlePassMission::WeakPtr>	BattlePassMissionMap;

private :
	INT64		m_UserID = 0LL;
	INT64		m_nRefreshTime = 0LL;
	BattlePassMap			repo_BattlePass;
	BattlePassMissionMap	repo_Mission;


public:
	auto GetPassRepository() -> decltype(repo_BattlePass)& { return repo_BattlePass; }
	auto GetMissionRepository() -> decltype(repo_Mission)& { return repo_Mission; }

	void	SetDirty() { m_nRefreshTime = -1LL; }
	INT64	GetRefreshTime() const { return m_nRefreshTime; }
	void	SetRefreshTime(INT64 value) { m_nRefreshTime = value; }

	void	SetUnlockWeekMission();
	
	void							SetUserID(const INT64 userID) { m_UserID = userID; }
	INT64							GetUserID() const { return m_UserID; }
	
	BattlePassDatum::SharedPtr		SeekDatum(INT32 nKey);
	CBattlePass::SharedPtr			Insert(CBattlePass::SharedPtr newData);
	void							Delete(INT64 nKey);
	CBattlePass::SharedPtr			Seek(INT32 nKey) { const auto spDatum = SeekDatum(nKey); return (spDatum) ? spDatum->pBattlePass : nullptr; }
	bool							UpdateQuestCacheMap();
	
	void							UpdateEvent(INT32 conditionKind, INT64 conditionValue, INT64 value, CQuestManager::SETTYPE setType, OUT CBattlePassMission::UpdateMissionInfos& vecNeedUpdate);
	void							AddPassPoint(CUser* const pUser, INT32 i32ItemKind, INT32 i32ItemCount);
	void							AdjustSeasonalBuff(CUser* const pUser);
	
};

struct CBattlePassHelper
{
	static INT32 GetWeekDay();

	static void OnUserCreate(std::shared_ptr<CUser> const pUser);
	static void OnUserLogin(std::shared_ptr<CUser> const pUser);
	static void OnUserComeback(std::shared_ptr<CUser> const pUser);
	static void OnUserLevelUp(CUser* const pUser);

	static void OnPurchaseComplete(CUser* const pUser, const INT32 productKind, INT64 checkTime = -1);
	
	static bool CheckRefreshTime(CUser* const pUser, bool sendClient = false);
	static bool CheckPassExpired(CUser* const pUser, const INT64 curTime);
	static bool CheckPassOpen(CUser* const pUser, const INT64 curTime);
	static bool CheckPassMissionOpen(CUser* const pUser, const INT64 curTime);
	static bool CheckPassMissionExpire(CUser* const pUser, const INT64 curTime);
	static void CheckPassPurchaseInApp(CUser* const pUser, const INT64 curTime);
	static bool RegistQuest(CUser* const pUser, CBattlePass::SharedPtr newData);

	// 몇 주차 미션까지 해금했는지 검사하기 위한 함수
	static void CheckUnlockWeekMission(CUser* const pUser);
	
	static void OnQuestEvent(INT64 userID, INT32 condKind, INT64 condValue, INT64 value, CQuestManager::SETTYPE settype);
	
	static bool Send_GS_BATTLEPASS_INFO_NFY(CUser* const pUser, const INT64 passid = 0);

	static bool OnRecvGS_BATTLEPASS_MISSION_COMPLETE_REQ(CUser* pUser, const void* pData);
	static bool OnRecvGS_BATTLEPASS_TIER_REWARD_REQ(CUser* pUser, const void* pData);
	static bool OnRecvGS_BATTLEPASS_TIER_PURCHASE_REQ(CUser* pUser, const void* pData);
	static bool OnRecvGS_BATTLEPASS_PASS_PURCHASE_REQ(CUser* pUser, const void* pData);
	
	static bool P_BATTLEPASS_LOAD(const std::vector<INT64> userIDs, const bool isServerStart);
	static bool P_BATTLEPASS_MISSION_UPSERT_TV(CUser* const pUser, CBattlePassMission::UpdateMissionInfos vecMissionUpdate, const BOOL isMissionNewOpen = false, const BOOL isOnQuestEvent = false);
	static bool P_BATTLEPASS_MISSION_UPSERT_TV_By_MissionComplete(CUser* const pUser, CBattlePassMission::UpdateMissionInfo missionUpdate, CBattlePassPoint::UpdatePassInfo updatePassPoint, CBattlePassMission::UpdateMissionInfo rollBackMission, CBattlePassPoint::UpdatePassInfo rollBackPassPoint);
	static bool P_BATTLEPASS_PURCHASE_Tier(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup, const BattlePass::ePassType passType, const INT64 targetPoint, const INT64 useGem, const INT64 purchaseReasonKind, const INT64 prePoint, CBattlePassPoint::RollbackPassInfo rollBackPoint);
	static bool P_BATTLEPASS_PURCHASE_Pass(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup, const BattlePass::ePassType passType, const INT64 targetPoint, const INT64 useGem, const INT64 purchaseReasonKind, const INT64 curGem, CBattlePassPoint::RollbackPassInfo rollBackPoint);
	static bool P_BATTLEPASS_PURCHASE_Pass_By_WAS(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup, const BattlePass::ePassType passType, const INT64 targetPoint, CBattlePassPoint::RollbackPassInfo rollBackPoint);

	static bool P_BATTLEPASS_REWARD_SET_TV(CUser* const pUser, const INT32 passKind, CBattlePassReward::UpdateRewardInfos vecPassRewardUpdate);
	static bool P_BATTLEPASS_SCHEDULE_UPSERT_TV(CUser* const pUser, const INT64 curTime, CBattlePass::UpdateScheduleInfos vecScheduleUpdate, std::vector<BASE::REWARDITEM> vecSetItem, std::vector<BASE::REWARDITEM> vecRollBackItem);

	static bool P_BATTLEPASS_ADD_PASS_POINT(CUser* const pUser, const INT32 passKind, const INT32 passRound, const BattlePass::ePassType passType, const INT64 totalPassPoint, const INT32 deleteItemKind, const INT32 deleteItemCount);

	static void SetBuffItem(INT64 i64UserID, const INT32 passKind, const INT32 iItemKind);

	static bool IsApplyADSkipBuff_By_PrimePass(INT64 i64UserID);
};


