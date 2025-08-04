#pragma once

namespace BattlePass
{

	const INT32 PURCHASE_PASS_ID = 910030;
	const INT32 PURCHASE_TIER_ID = 910010;
	const INT32 BATTLEPASS_PACKAGE_GROUPCODE = 900000;

	const INT32 TIER_TO_POINT = 100;
	
	enum class eScheduleType 
	{
		NONE = 0,
		CASTLELEVEL = 1,
		MONTHLY = 2,
		END,
	};

	enum class ePassMissionType
	{
		NONE = 0,
		DAY = 1,
		WEEK = 7,
		PRIME = 14,
		SEASON = 30,
		END,
	};
	enum class eRewardType
	{
		NONE = 0,
		FREE = 1,
		PAID = 2,
		END,
	};
	enum class ePassState
	{
		NONE = 0,
		PROCESSING = 1,
		DAYOFF = 2,
	};

	enum class ePassType
	{
		NONE = 0,
		FREE = 1,
		PAID = 2,
		END,
	};

	enum class ePassMissionState
	{
		NONE = 0,
		PROCESSING = 1,
		COMPLETE = 2,
		REWARDED = 3,
		EXPIRE = 4,
		END,
	};


	struct BattlePassScheduleInfo
	{
		typedef	std::shared_ptr<BattlePassScheduleInfo>		SharedPtr;
		typedef std::weak_ptr<BattlePassScheduleInfo>		WeakPtr;


		INT32 PassKind = 0;
		INT32 MinServerAge = 0;
		eScheduleType ScheduleType = eScheduleType::NONE;
		INT32 ScheduleValue = 0;
		INT32 ProgressDay = 0;
		INT32 Dayoff = 0;
		INT32 MinLevel = 0;
		std::vector<BASE::REWARDITEM> vecGiveItem;

	};

	struct BattlePassRewardGroupInfo
	{
		typedef	std::shared_ptr<BattlePassRewardGroupInfo>		SharedPtr;
		typedef std::weak_ptr<BattlePassRewardGroupInfo>		WeakPtr;

		INT32 PassKind = 0;
		INT32 RewardGroup = 0;
		INT32 ServerAgeMin = 0;
		INT32 ServerAgeMax = 0;
	};

	struct BattlePassPointinfo
	{
		typedef	std::shared_ptr<BattlePassPointinfo>	SharedPtr;
		typedef std::weak_ptr<BattlePassPointinfo>		WeakPtr;

		INT32 PassKind = 0;
		INT32 RewardGroup = 0;
		INT32 PassTier = 0;
		INT32 NeedPoint = 0;
		INT32 NeedGem = 0;
		INT32 PremiumReward = 0;

		std::vector<BASE::REWARDITEM> vecFreeReward;
		std::vector<BASE::REWARDITEM> vecPaidReward;

		INT8 NotiFlag = 0;
	};

	struct BattlePassInfo
	{
		typedef	std::shared_ptr<BattlePassInfo>		SharedPtr;
		typedef std::weak_ptr<BattlePassInfo>		WeakPtr;

		INT32 PassKind = 0;
		INT32 NeedGem = 0;
		INT32 SaleRate = 0;
		INT32 TierUpto = 0;

		std::vector<INT32> vecSeasonalBuff;
		std::vector<BASE::REWARDITEM> vecGuildPresent;
		std::vector<BASE::REWARDITEM> vecSpecialBenefit;

		INT32 PackageGroupCode = 0;
		INT32 PackageKind = 0;
	};

	struct BattlePassMissionInfo
	{
		typedef	std::shared_ptr<BattlePassMissionInfo>		SharedPtr;
		typedef std::weak_ptr<BattlePassMissionInfo>		WeakPtr;

		INT64 Missionkind = 0;
		INT32 PassKind = 0;
		ePassMissionType MissionScheduleType = ePassMissionType::NONE;
		INT32 MissionScheduleValue = 0;
		ePassType MissionPassType = ePassType::NONE;
		INT32 CastleLevelMin = 0;
		INT32 CastleLevelMax = 0;
		INT32 MissionGroup = 0;
		INT32 ConditionChronicle = 0;
		INT32 ConditionKind = 0;
		INT32 ConditionValue = 0;
		INT64 TargetValue = 0;
		INT64 RewardPoint = 0;

		std::vector<BASE::REWARDITEM> vecRewardItems;

		bool IsCastleLevelInRange(INT64 castleLevel) const {	return CastleLevelMin <= castleLevel && castleLevel <= CastleLevelMax; }
	};

	class CBattlePassManager : public nx::Singleton<CBattlePassManager>
	{	

	private:
		// PassKind, ScheduleInfo
		std::unordered_map<INT32, BattlePassScheduleInfo::SharedPtr>												repo_Schedule;
		// PassKind, <RewardGroup,RewardGroupInfo>
		std::unordered_map<INT32,std::unordered_map<INT32, BattlePassRewardGroupInfo::SharedPtr>>					repo_RewardGroup;
		// <PassKind, RewardGroup>, <<Tier> PointInfo>
		std::unordered_map<std::pair<INT32, INT32>, std::unordered_map<INT32, BattlePassPointinfo::SharedPtr>>		repo_Point;
		// PassKind, PassInfo
		std::unordered_map<INT32, BattlePassInfo::SharedPtr>														repo_Pass;
		// PassKind, <<Missionkind >MissionInfo>
		std::unordered_map<INT32, std::unordered_map<INT64,BattlePassMissionInfo::SharedPtr>>						repo_Mission;
		// PassKind, <MISSION_TYPE, MissionInfo>
		std::unordered_map<INT32, std::unordered_multimap<ePassMissionType, BattlePassMissionInfo::WeakPtr>>		repo_Type_Mission;
		
		// 서버 기동 시 유저 데이터
		std::stack<INT64> m_userIDs;


		
	private:
		const BattlePassScheduleInfo::SharedPtr				SeekSchedule(const INT32 passkind) const;
		const BattlePassRewardGroupInfo::SharedPtr			SeekRewardGroup(const INT32 passkind, const INT32 groupid) const;
		const BattlePassPointinfo::SharedPtr				SeekPointInfo(const INT32 passkind, const INT32 groupid, const INT32 tier) const;
		const BattlePassInfo::SharedPtr						SeekPassInfo(const INT32 passkind) const;
		const BattlePassMissionInfo::SharedPtr				SeekMissionInfo(const INT32 passkind, const INT64 missionkind) const;

		BattlePassScheduleInfo::SharedPtr					InsertSchedule(BattlePassScheduleInfo::SharedPtr newInfo);
		BattlePassRewardGroupInfo::SharedPtr				InsertRewardGroup(BattlePassRewardGroupInfo::SharedPtr newInfo);
		BattlePassPointinfo::SharedPtr						InsertPointInfo(BattlePassPointinfo::SharedPtr newInfo);
		BattlePassInfo::SharedPtr							InsertPassInfo(BattlePassInfo::SharedPtr newInfo);
		BattlePassMissionInfo::SharedPtr					InsertMissionInfo(BattlePassMissionInfo::SharedPtr newInfo);

	public:
		const BattlePassScheduleInfo::SharedPtr				GetSchedule(const INT32 passkind) const;
		const BattlePassRewardGroupInfo::SharedPtr			GetRewardGroup(const INT32 passkind, const INT32 groupid) const;
		const BattlePassPointinfo::SharedPtr				GetPointInfo(const INT32 passkind, const INT32 groupid, const INT32 tier) const;
		const BattlePassInfo::SharedPtr						GetPassInfo(const INT32 passkind) const;
		const BattlePassInfo::SharedPtr						GetPassInfoByPackageKind(const INT32 packageKind) const;
		const BattlePassMissionInfo::SharedPtr				GetMission(const INT32 passkind, const INT64 missionkind) const;
		auto												GetMissionInfos(const INT32 passkind)->std::unordered_map<INT64, BattlePassMissionInfo::SharedPtr>*;
		auto												GetMissionInfosByType(const INT32 passkind)->std::unordered_multimap<ePassMissionType, BattlePassMissionInfo::WeakPtr>*;

		INT64												GetPassMaxPoint(const INT32 passkind, const INT32 rewardGroup);

		bool												CheckPassBuffStat(const INT32 passKind, BuffContents::eBUFF_STAT_KIND _buffStatKind) const;

		auto& GetAllSchedule() { return repo_Schedule; }

		const INT32 GetRewardGroupIndex(const INT32 passkind, const INT64 curTime);

		INT64 GetNeedGem(const INT64 curPoint, const INT64 targetPoint);

		INT32 GetUTCLastDayOfMonth(const INT64 curTime);

		bool HasItemInVecGiveItem(INT32 passKind, INT32 itemKind);


	public:
		void Send_DB_BATTLEPASS_LOAD_START();
		void Send_DB_BATTLEPASS_LOAD_REQ();
		INT8 Recv_DB_BATTLEPASS_LOAD_ACK(const void* pData);


	public:
		//NDT Load

		SRESULT LoadSchedule(NDataReader& data);
		SRESULT LoadRewardGroup(NDataReader& data);
		SRESULT LoadPoint(NDataReader& data);
		SRESULT LoadPass(NDataReader& data);
		SRESULT LoadMission(NDataReader& data);
	};
}




