#pragma once

namespace CustomEvent
{
	const int PAGE_LOAD_MAX = 512;
	const int BINGO_ROW_COUNT = 5;
	const int MAX_BINGO_NUM = BINGO_ROW_COUNT * BINGO_ROW_COUNT;

	enum class eWARDICE_CITY_TYPE
	{
		NONE = 0,
		PARIS = 1,
		LONDON = 2,
		BERLIN = 3,
		MOSKVA = 4,
		ROMA = 5,
	};

	enum class eCustomEventDifficulty
	{
		NONE = 0,
		NORMAL = 1,
		DOUBLE = 2,
		TRIPLE = 3,
	};

	enum class eEventType : UINT8
	{
		NONE		= 0,
		BINGO		= 1,
		ROULETTE	= 2,
		TIMINGGAME  = 3,
		WARDICE		= 4,
		//CHANGE    = 3,
		//DICE      = 3,
		//DARTS     = 3,
		END,
	};

	enum class eScheduleType : UINT8
	{
		NONE       = 0,
		UTC_DATE   = 1,
		SERVER_AGE = 2,
		USER_AGE   = 3,
	};

	enum class eEventState : UINT8
	{
		NONE       = 0,
		PROCESSING = 1,
		EXPIRED    = 2,
		ITEMDELETE = 3,
	};

	enum class eCustomEventMissionState
	{
		NONE = 0,
		PROCESSIONG = 1,
		COMPLETE = 2,
		REWARDED = 3,
	};

	struct ScheduleInfo
	{
		typedef	std::shared_ptr<ScheduleInfo>	SharedPtr;
		typedef std::weak_ptr<ScheduleInfo>		WeakPtr;

		// WWM_CustomEvent_Scheduling
		INT64			EventID = 0;
		UINT8			Active = 0;

		eScheduleType	ScheduleType = eScheduleType::NONE;
		INT64			ScheduleValue = 0;
		INT32			EventDuration = 0;
		INT32			QuestGroup = 0;
		INT32			RewardDuration = 0;

		INT32			Min_ServerAge = 0;
		INT32			Max_ServerAge = 0;
		INT32			Min_UserAge = 0;
		INT32			Max_UserAge = 0;
		INT32			Min_Level = 0;
		INT32			Max_Level = 0;
		INT32			Min_PayingLevel = 0;
		INT32			Max_PayingLevel = 0;

		bool            TargetServer;
		bool            BanServer;

		eEventType		EventType = eEventType::NONE;
		UINT8			TapInfo = 0;
		UINT8			ColorIndex = 0;

		// WWM_CustomEvent_Season
		INT32			KeyItemKind = 0;
		INT32			KeyItemCount = 0;

		std::wstring	BGImagePath;
		std::wstring	NumberImagePath;
		std::wstring	IconKey;
		std::wstring	IconSprite;
		std::wstring	IconAtlas;
		std::wstring	HelpKey;
		std::wstring	TitleKey;

		// etc
		INT32			MaxRound = 0;
	};

	struct RoundInfo
	{
		typedef	std::shared_ptr<RoundInfo>		SharedPtr;
		typedef std::weak_ptr<RoundInfo>		WeakPtr;

		INT64			EventID = 0;
		INT32			Round = 0;
		INT32			RoundRewardGroupID = 0;
		INT32			FixRewardGroupID = 0;
		std::vector<INT32>	RoundGoal;

		std::wstring	RoundImagePath;
	};

	struct RewardInfo
	{
		typedef	std::shared_ptr<RewardInfo>		SharedPtr;
		typedef std::weak_ptr<RewardInfo>		WeakPtr;

		INT32			RewardGroupID = 0;
		INT32			RewardType = 0;		// BINGO에서는 빙고번호, ROULETTE에서는 라운드 내 포인트 달성 단계
		GAME::eNATION	Nation = GAME::eNATION::eNATION_NONE;

		std::vector<BASE::REWARDITEM> RewardItems;
	};

	struct ProbabilityInfo
	{
		typedef	std::shared_ptr<ProbabilityInfo>	SharedPtr;
		typedef std::weak_ptr<ProbabilityInfo>		WeakPtr;

		INT64			EventID = 0;
		INT32			ProbKey_1 = 0;
		INT32			ProbKey_2 = 0;
		INT32			Probability = 0;
	};
	
	struct TextInfo
	{
		typedef	std::shared_ptr<TextInfo>		SharedPtr;
		typedef std::weak_ptr<TextInfo>			WeakPtr;

		std::unordered_map<std::wstring, std::wstring> LanguageMap;
	};

	struct DeleteItemInfo
	{
		typedef	std::shared_ptr<DeleteItemInfo>	SharedPtr;
		typedef std::weak_ptr<DeleteItemInfo>		WeakPtr;

		INT64			EventID = 0;
		INT64			ItemDeleteTime = 0;
		std::wstring     TitleKey;
		std::wstring     ContentsKey;


		std::vector<INT64> KeyItemKinds;
		std::vector<BASE::REWARDITEM> RewardItems;
	};

	struct QuestInfo
	{
		typedef	std::shared_ptr<QuestInfo>	SharedPtr;
		typedef std::weak_ptr<QuestInfo>		WeakPtr;

		INT32 GroupID = 0;
		INT64  Day = 0;
		INT32 QuestID = 0;
		INT32 ConditionKind = 0;
		INT64 ConditionValue = 0;
		INT64 TargetValue = 0;
		
		std::wstring	PointHelpKey;
		std::vector<BASE::REWARDITEM> RewardItems;
	};

	struct ValueInfo
	{
		//타이밍 게임
		//도탄점수 / 명중점수 / 대성공점수 / 사격횟수 / 커서 이동속도 / 성공바 크기 / 대성공 바 크기 / 성공 추가 보상 갯수 / 대성공 추가 보상 갯수 / 아이템 Kind


		typedef	std::shared_ptr<ValueInfo>		SharedPtr;
		typedef std::weak_ptr<ValueInfo>		WeakPtr;

		INT64 EventID = 0;
		UINT8 Difficulty = 0;
		INT32 Value1 = 0;
		INT32 Value2 = 0;
		INT32 Value3 = 0;
		INT32 Value4 = 0;
		INT32 Value5 = 0;
		INT32 Value6 = 0;
		INT32 Value7 = 0;
		INT32 Value8 = 0;
		INT32 Value9 = 0;
		INT32 Value10 = 0;
	};

	// eventID, ScheduleInfo
	typedef std::unordered_map<INT64, ScheduleInfo::SharedPtr>																	ScheduleInfoMap;

	// eventID, (round, RoundInfo)
	typedef std::unordered_map<INT64, std::unordered_map<INT32, RoundInfo::SharedPtr>>											RoundInfoMap;

	// rewardGroupID, (nation (rewardType, RewardInfo))
	typedef std::unordered_map<INT32, std::unordered_map<GAME::eNATION, std::unordered_map<INT32, RewardInfo::SharedPtr>>>		RewardInfoMap;

	// eventID, (key, ProbabilityInfo)
	typedef std::unordered_map<INT64, std::unordered_map<std::tuple<INT32, INT32>, ProbabilityInfo::SharedPtr>>					ProbabilityInfoMap;

	// eventID, ScheduleInfo
	typedef std::unordered_map<INT64, DeleteItemInfo::SharedPtr>																DeleteItemInfoMap;

	// groupID,  (QuestID, QuestInfo ) 
	typedef std::unordered_map<INT64, std::unordered_map<INT64, QuestInfo::SharedPtr>>											QuestInfoMap;

	// EventID, (Difficulty, ValueInfo)
	typedef std::unordered_map<INT64, std::unordered_map<eCustomEventDifficulty,ValueInfo::SharedPtr>>							ValueInfoMap;
		
	class CCustomEventInfoManager
	{
	private:
		ScheduleInfoMap		repo_Schedule;
		RoundInfoMap		repo_Round;
		RewardInfoMap		repo_Reward;
		ProbabilityInfoMap	repo_Probability;
		DeleteItemInfoMap	repo_DeleteItem;
		QuestInfoMap		repo_Quest;
		ValueInfoMap		repo_ValueInfo;
		
		std::unordered_map<GAME::eLANGUAGE_CODE, TextInfo::SharedPtr>					repo_Text;

		std::unordered_map<INT64, ScheduleInfo::WeakPtr>	map_CurSchedule;

		INT64 m_lUpdateTime = -1LL;

		// 서버 기동 시 유저 로딩을 위한 stack
		std::stack<INT64> m_UserIDs_ServerStartLoading;

	private:
		void								Clear();
		const ScheduleInfo::SharedPtr		SeekSchedule(const INT64 eventID) const;
		const RoundInfo::SharedPtr			SeekRound(const INT64 eventID, const INT32 round) const;
		const RewardInfo::SharedPtr			SeekReward(const INT32 groupID, const GAME::eNATION nation, const INT32 rewardType) const;
		const ProbabilityInfo::SharedPtr	SeekProbability(const INT64 eventID, const std::tuple<INT32, INT32> key) const;
		const DeleteItemInfo::SharedPtr		SeekDeleteItemInfo(const INT64 eventID) const;
		const QuestInfo::SharedPtr			SeekQuest(const INT64 groupID, const INT64 questID) const;
		const ValueInfo::SharedPtr			SeekValue(const INT64 eventID, const eCustomEventDifficulty difficulty) const;
		
		ScheduleInfo::SharedPtr		InsertSchedule(ScheduleInfo::SharedPtr newInfo);
		RoundInfo::SharedPtr		InsertRound(RoundInfo::SharedPtr newInfo);
		RewardInfo::SharedPtr		InsertReward(RewardInfo::SharedPtr newInfo);
		ProbabilityInfo::SharedPtr	InsertProbability(ProbabilityInfo::SharedPtr newInfo);
		TextInfo::SharedPtr			InsertText(GAME::eLANGUAGE_CODE langCode, std::wstring textKey, std::wstring textValue);
		DeleteItemInfo::SharedPtr	InsertDeleteItem(DeleteItemInfo::SharedPtr newInfo);
		QuestInfo::SharedPtr		InsertQuest(QuestInfo::SharedPtr newInfo);
		ValueInfo::SharedPtr		InsertValue(ValueInfo::SharedPtr newInfo);

	public:
		static std::shared_ptr<CCustomEventInfoManager> Instance() { return CRuntimeEventInfoManager::Instance()->Get_CustomEventInfoManager(); }

		auto GetCurEventMap() -> decltype(map_CurSchedule)& { return map_CurSchedule; }
		const ScheduleInfo::SharedPtr		GetScheduleInfo(const INT64 eventID) const;
		const RoundInfo::SharedPtr			GetRoundInfo(const INT64 eventID, const INT32 round) const;
		const RewardInfo::SharedPtr			GetRewardInfo(const INT32 groupID, const GAME::eNATION nation, const INT32 rewardType) const;
		const ProbabilityInfo::SharedPtr	GetProbabilityInfo(const INT64 eventID, const INT64 mainKey, const INT64 subKey) const;
		const TextInfo::SharedPtr			GetTextInfo(const GAME::eLANGUAGE_CODE langCode) const;
		const DeleteItemInfo::SharedPtr		GetDeleteItemInfo(const INT64 eventID) const;
		const QuestInfo::SharedPtr			GetQuestInfo(const INT64 eventID, const INT64 questID) const;
		const ValueInfo::SharedPtr			GetValueInfo(const INT64 eventID, const eCustomEventDifficulty difficulty) const;
		
		void GetRoundInfos(const INT64 eventID, OUT std::vector<RoundInfo::SharedPtr>& out_vecRoundInfos);
		void GetRewardInfos(const INT64 eventID, const INT32 round, const GAME::eNATION nation, OUT std::vector<std::tuple<INT32, RewardInfo::SharedPtr>>& out_vecRewardInfos);
		void GetCalendarRewardInfos(const INT64 eventID, const INT32 round, const GAME::eNATION nation, OUT std::vector<std::tuple<INT32, RewardInfo::SharedPtr>>& out_vecRewardInfos);
		void GetProbabilityInfos(const INT64 eventID, OUT std::vector<ProbabilityInfo::SharedPtr>& out_vecProbabilityInfos);
		auto GetQuestInfos(const INT64 groupID)->std::unordered_map<INT64, QuestInfo::SharedPtr>*;
		auto GetValueInfos(const INT64 eventID)->std::unordered_map<eCustomEventDifficulty, ValueInfo::SharedPtr>*;
		INT64 GetEventIDByKeyItem(const INT32 keyItemKind);
		
		void Update();
		const bool IsReady() const;
		bool RefreshCurrentEventInfo(const INT64 curTime);
		void SetDirty();
		void SetDirtyAllUser();

		INT8 GetTimingGameTotalCount(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameSuccessBarSize(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameGreatSuccessBarSize(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameFailAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameSuccessAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameGreatSuccessAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty);
		INT32 GetTimingGameKeyItemKind(const INT64 eventID, const eCustomEventDifficulty difficulty);

		void GetEventCalendarInfo(const INT64 calendarStartTime, const INT64 calendarEndTime, CUser* const pUser, OUT std::vector<EventCalendarData>& outInfo);

		INT32 GetWarDiceMagnification(const INT64 eventID, const INT32 round ,const INT32 block);
		
	public:
		void P_CUSTOM_EVENT_INFO_LOAD(bool isServerStart);
		void P_CUSTOM_EVENT_TEXT_LOAD(bool isServerStart);

		void Send_DB_CUSTOM_EVENT_INFO_LOAD_REQ();
		INT8 Recv_DB_CUSTOM_EVENT_INFO_LOAD_ACK(const void* pData);
		void Send_DB_CUSTOM_EVENT_TEXT_LOAD_REQ();
		INT8 Recv_DB_CUSTOM_EVENT_TEXT_LOAD_ACK(const void* pData);

		void Send_DB_CUSTOM_USERDATA_LOAD_START();
		void Send_DB_CUSTOM_USERDATA_LOAD_REQ();
		INT8 Recv_DB_CUSTOM_USERDATA_LOAD_ACK(const void* pData);

		void Recv_GS_CUSTOM_EVENT_TEXT_GET_REQ(CUser* const pUser, const void* pData);

	private:
		void UpdateTextAllUser();
	};
}

