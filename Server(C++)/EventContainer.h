#pragma once

class CUser;

typedef std::array<std::array<bool, CustomEvent::BINGO_ROW_COUNT>, CustomEvent::BINGO_ROW_COUNT> BingoBoardArray;

class CCustumEventReward : public std::enable_shared_from_this<CCustumEventReward>
{
public:
	using SharedPtr = std::shared_ptr<CCustumEventReward>;
	using WeakPtr = std::weak_ptr<CCustumEventReward>;

public:
	INT32	m_Round = 0;
	INT32	m_RewardType = 0;	// BINGO에서는 빙고번호, ROULETTE에서는 라운드 내 포인트 달성 단계
	INT32	m_RewardValue = 0;
};

class CCustomEventQuest : public std::enable_shared_from_this<CCustomEventQuest>
{
public:
	using SharedPtr = std::shared_ptr<CCustomEventQuest>;
	using WeakPtr = std::weak_ptr<CCustomEventQuest>;

public:
	INT64 m_EventID = 0;
	INT64 m_QuestID = 0;
	INT64 m_CurValue = 0;
	CustomEvent::eCustomEventMissionState m_MissionState = CustomEvent::eCustomEventMissionState::NONE;
};

class CCustumEventBase : public std::enable_shared_from_this<CCustumEventBase>
{
public:
	using SharedPtr = std::shared_ptr<CCustumEventBase>;
	using WeakPtr = std::weak_ptr<CCustumEventBase>;

public:
	INT64	m_EventID = 0;
	INT64	m_StartTime = 0;
	INT64	m_EndTime = 0;

	INT32	m_Round = 0;
	INT32	m_RewardKind = 0;

	CustomEvent::eEventState m_State = CustomEvent::eEventState::NONE;

	// (round, rewardType), CCustumEventReward
	std::unordered_map<std::tuple<INT32, INT32>, CCustumEventReward::SharedPtr>		m_RewardMap;

	// (QuestID, CCustomEventQuest)
	std::unordered_map<INT64, CCustomEventQuest::SharedPtr>							m_QuestMap;
	
	bool SetReward(const INT32 round, const INT32 rewardType, const INT32 rewardValue);
	bool IsRewarded(const INT32 round, const INT32 rewardType);
	void GetReward(const INT32 round, OUT std::vector<INT32>& out_RewardTypes);
	INT32 GetRewardValue(const INT32 round, const INT32 rewardType);

	virtual bool SetProgress(const INT32 value) { return false; };
	virtual void ClearProgress() {};
};

class CBingoEvent : public CCustumEventBase
{
public:
	using SharedPtr = std::shared_ptr<CBingoEvent>;
	using WeakPtr = std::weak_ptr<CBingoEvent>;

public:
	BingoBoardArray m_BingoBoard = { 0, };

	virtual bool SetProgress(const INT32 bingoNum);
	virtual void ClearProgress();
};

class CRouletteEvent : public CCustumEventBase
{
public:
	using SharedPtr = std::shared_ptr<CRouletteEvent>;
	using WeakPtr = std::weak_ptr<CRouletteEvent>;

public:
	INT32	m_Score = 0;

	virtual bool SetProgress(const INT32 value);
	virtual void ClearProgress();
};
class CTimingGameEvent : public CCustumEventBase
{
public:
	using SharedPtr = std::shared_ptr<CTimingGameEvent>;
	using WeakPtr = std::weak_ptr<CTimingGameEvent>;

	INT32   m_Score = 0;
	INT32	m_SuccessBarX = 0;
	INT32   m_GreatSuccessBarX = 0;
	INT32   m_Round = 0;
	INT32   m_BounsPoint = 0;

	virtual void ClearProgress();
	virtual bool SetProgress(const INT32 successBar, const INT32 greatSuccessBar, const INT32 round, const INT32 bounsPoint);
};
class CWarDiceEvent : public CCustumEventBase
{
public:
	using SharedPtr = std::shared_ptr<CWarDiceEvent>;
	using WeakPtr = std::weak_ptr<CWarDiceEvent>;

	INT32 m_DiceBlock = 0;

	virtual void ClearProgress();
	virtual bool SetProgress(const INT32 block);

};

class CCustomEventContainer
{
public:
	struct EventDatum
	{
		typedef std::shared_ptr<EventDatum>		SharedPtr;
		typedef std::weak_ptr<EventDatum>		WeakPtr;

		CCustumEventBase::SharedPtr		pEvent = nullptr;
		std::unordered_map<INT64, EventDatum::SharedPtr>::iterator	iter_Event;
	};

	typedef std::unordered_map<INT64, EventDatum::SharedPtr>		SourceMap;
	typedef std::unordered_multimap<INT32, CCustomEventQuest::WeakPtr>	QuestMap;

private:
	INT64		m_UserID = 0LL;
	SourceMap	repo_Event;
	QuestMap	repo_Quest;
	INT64		m_nRefreshTime = -1LL;

public:
	auto GetEventRepository() -> decltype(repo_Event)& { return repo_Event; }
	auto GetQuestRepository() -> decltype(repo_Quest)& { return repo_Quest; }

	EventDatum::SharedPtr			SeekDatum(INT64 nKey);
	CCustumEventBase::SharedPtr		Insert(CCustumEventBase::SharedPtr newData);
	void							Delete(INT64 nKey);
	CCustumEventBase::SharedPtr		Seek(INT64 nKey) { auto spDatum = SeekDatum(nKey); return (spDatum) ? spDatum->pEvent : nullptr; }

	void	SetUserID(const INT64 userID)	{ m_UserID = userID; }
	void	SetDirty()						{ m_nRefreshTime = -1LL; }
	INT64	GetRefreshTime() const			{ return m_nRefreshTime; }
	void	SetRefreshTime(INT64 value)		{ m_nRefreshTime = value; }

	bool	UpdateQuestCacheMap();

public:

	set<INT64> GetEventID_By_ScheduleType(CustomEvent::eScheduleType eScheduleType);
};

struct CustomEventHelper
{
	// OnEvent - Common
	static void OnUserCreate(std::shared_ptr<CUser> const pUser);
	static void OnUserLogin(std::shared_ptr<CUser> const pUser);
	static void OnUserLevelUp(CUser* const pUser);
	static void OnUserComeback(std::shared_ptr<CUser> const pUser);
	static bool OnQuestEvent(INT64 userID, INT32 conditionKind, INT64 conditionValue, INT64 value, CQuestManager::SETTYPE settype);

	// Logic - Common
	static bool CheckRefreshTime(CUser* const pUser, bool sendClient = false);
	static bool CheckEventExpired(CUser* const pUser, const INT64 curTime);
	static bool CheckEventOpen(CUser* const pUser, const INT64 curTime);
	static bool CheckDeleteItem(CUser* const pUser, const INT64 curTime);

	static bool RegistQuest(CCustumEventBase::SharedPtr newData);

	// DB Procedure - Common
	static bool P_CUSTOM_EVENT_LOAD(const std::vector<INT64> userIDs, const bool sendClient, const bool serverStart);
	static bool P_CUSTOM_EVENT_SCHEDULE_SET(CUser* const pUser, const std::vector<std::tuple<INT64, INT64, INT64>> vecInfos);
	static bool P_CUSTOM_EVENT_UPDATE_SelectRewardKind(CUser* const pUser, const INT64 eventID, const INT32 rewardKind);
	static bool P_CUSTOM_EVENT_COMPLETE(CUser* const pUser, const INT64 eventID);
	static bool P_CUSTOM_EVENT_ITEMDELETE(CUser* const pUser, const INT64 eventID);
	static bool P_CUSTOMEEVENT_QUEST_SET_TV(CUser* const pUser, std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecInfos, const bool sendClient = false);

	// Send Client - Common
	static bool Send_GS_CUSTOM_EVENT_INFO_NFY(CUser* const pUser, const INT64 eventID = 0);


	// DB Procedure - TimingGame
	static bool OnRecv_GS_CUSTOM_EVENT_PLAY_REQ_PlayTimingGame(CUser* const pUser, const INT64 eventID, const UINT8 difficulty);
	static bool OnRecvGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(CUser* pUser, const void* pData);
	static void OnRecvGS_CUSTOM_EVENT_MISSION_REWARD_REQ(CUser* pUser, const void* pData);

	// Logic - Bingo
	static bool ConvertBingoBoardFromBitFlag(const INT32 progress, OUT BingoBoardArray& bingoBoard);
	static bool ConvertBingoBoardToBitFlag(const BingoBoardArray& bingoBoard, OUT INT32& progress);
	static bool ConvertBingoBoardToVector(const BingoBoardArray& bingoBoard, OUT std::vector<INT32>& vecBingoNum);
	static bool CheckComplete(const BingoBoardArray& bingoBoard);
	static bool CheckBingoCount(const BingoBoardArray& bingoBoard, const INT32 bingoCount);
	static INT32 GetRandomBingoNumber(const BingoBoardArray& bingoBoard, OUT std::set<INT32>& rewardTypes);

	// DB Procedure - Bingo
	static bool P_CUSTOM_EVENT_UPDATE_PlayBingo(CUser* const pUser, const INT64 eventID, const INT32 bingoCount);

	// Logic - Roulette
	static std::optional<std::tuple<INT32, INT32>> GetRandomRouletteNumber(
		const INT64 eventID, const INT32 curRound, const INT32 prevScore, const GAME::eNATION nation, OUT std::set<std::tuple<INT32, INT32>>& rewardTypes);

	// DB Procedure - Roulette
	static bool P_CUSTOM_EVENT_UPDATE_PlayRoulette(CUser* const pUser, const INT64 eventID, const INT32 curScore);
	
	// DB Procedure - WarDice
	static bool P_CUSTOM_EVENT_UPDATE_PlayWarDice(CUser* const pUser, const INT64 eventID, const INT32 curScore);

	// Logic - WarDice
	static INT32 GetDiceNumber(const INT64 eventID, const INT32 curRound, const INT32 prevScore, const GAME::eNATION nation, OUT std::set<std::tuple<INT32, INT32>>& rewardTypes);

	// Cheat
	static void GM_CUSTOM_EVNET_PROGRESS_SET(CUser* const pUser, const INT64 eventID, const INT32 point);
	static bool P_GM_CUSTOM_EVENT_UPDATE_PlayRoulette(CUser* const pUser, const INT64 eventID, const INT32 point);

	static void GM_WARDICE_SOLDIER_MOVE(CUser* const pUser, const INT64 eventID, const INT32 moveCount);
	static void GM_CUSTOMEVENT_ROUND_SET(CUser* const pUser, const INT64 eventID, const INT32 round);

};

