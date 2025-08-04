#include "GameAfx.h"
#include "_DatabaseThreadManager/DBQUERY/GameDB.h"
#include "_DatabaseThreadManager/DBAGlobal.h"

namespace BattlePass 
{

	const BattlePassScheduleInfo::SharedPtr CBattlePassManager::SeekSchedule(const INT32 passkind) const
	{
		auto iterF = repo_Schedule.find(passkind);
		if (iterF == repo_Schedule.end())
			return nullptr;

		return (*iterF).second;
	}

	const BattlePassRewardGroupInfo::SharedPtr CBattlePassManager::SeekRewardGroup(const INT32 passkind, const INT32 groupid) const
	{
		auto iterF = repo_RewardGroup.find(passkind);
		if (iterF == repo_RewardGroup.end())
			return nullptr;

		auto& groupMap = (*iterF).second;
		auto iter2 = groupMap.find(groupid);

		if (iter2 == groupMap.end())
			return nullptr;



		return (*iter2).second;
	}

	const BattlePassPointinfo::SharedPtr CBattlePassManager::SeekPointInfo(const INT32 passkind, const INT32 groupid, const INT32 tier) const
	{
		auto iter1 = repo_Point.find(std::make_pair(passkind, groupid));
		if (iter1 == repo_Point.end())
			return nullptr;

		auto& pointMap = (*iter1).second;
		auto iter2 = pointMap.find(tier);

		if (iter2 == pointMap.end())
			return nullptr;

		return (*iter2).second;
	}

	const BattlePassInfo::SharedPtr CBattlePassManager::SeekPassInfo(const INT32 passkind) const
	{
		auto iterF = repo_Pass.find(passkind);
		if (iterF == repo_Pass.end())
			return nullptr;

		return (*iterF).second;
	}

	const BattlePassMissionInfo::SharedPtr CBattlePassManager::SeekMissionInfo(const INT32 passkind, const INT64 missionkind) const
	{
		auto iter1 = repo_Mission.find(passkind);
		if (iter1 == repo_Mission.end())
			return nullptr;

		auto& missionMap = (*iter1).second;

		auto iter2 = missionMap.find(missionkind);
		if (iter2 == missionMap.end())
			return nullptr;

		return (*iter2).second; 
	}

	BattlePassScheduleInfo::SharedPtr CBattlePassManager::InsertSchedule(BattlePassScheduleInfo::SharedPtr newInfo)
	{
		if (newInfo == nullptr)
			return nullptr;

		if (SeekSchedule(newInfo->PassKind) != nullptr)
			return nullptr;

		auto [iter_1, result_1] = repo_Schedule.emplace(newInfo->PassKind, newInfo);
		if (!result_1)
			return nullptr;

		return newInfo;
	}

	BattlePassRewardGroupInfo::SharedPtr CBattlePassManager::InsertRewardGroup(BattlePassRewardGroupInfo::SharedPtr newInfo)
	{
		if (newInfo == nullptr)
			return nullptr;
		
		if (SeekRewardGroup(newInfo->PassKind, newInfo->RewardGroup) != nullptr)
			return nullptr;

		auto iter1 = repo_RewardGroup.find(newInfo->PassKind);
		if (iter1 != repo_RewardGroup.end())
		{
			auto& groupMap = (*iter1).second;
			auto [iter_1, result_1] = groupMap.emplace(newInfo->RewardGroup, newInfo);
			if (!result_1)
				return nullptr;
		}
		else
		{
			std::unordered_map<INT32, BattlePassRewardGroupInfo::SharedPtr> groupmap;
			auto [iter_1, result_1] = groupmap.emplace(newInfo->RewardGroup, newInfo);
			if (!result_1)
				return nullptr;

			auto [iter_2, result_2] = repo_RewardGroup.emplace(newInfo->PassKind, groupmap);
			if (!result_2)
				return nullptr;
		}

		return newInfo;

	}

	BattlePassPointinfo::SharedPtr CBattlePassManager::InsertPointInfo(BattlePassPointinfo::SharedPtr newInfo)
	{
		if (newInfo == nullptr)
			return nullptr;

		auto pointInfoKey = std::make_pair(newInfo->PassKind, newInfo->RewardGroup);

		if (SeekPointInfo(pointInfoKey.first, pointInfoKey.second,newInfo->PassTier) != nullptr)
			return nullptr;

		auto iter1 = repo_Point.find(pointInfoKey);
		if (iter1 != repo_Point.end())
		{
			auto& pointMap = (*iter1).second;
			auto [iter_1, result_1] = pointMap.emplace(newInfo->PassTier, newInfo);
			if (!result_1)
				return nullptr;
		}
		else
		{
			std::unordered_map<INT32, BattlePassPointinfo::SharedPtr> pointMap;
			auto [iter_1, result_1] = pointMap.emplace(newInfo->PassTier, newInfo);
			if (!result_1)
				return nullptr;

			auto [iter_2, result_2] = repo_Point.emplace(pointInfoKey, pointMap);
			if (!result_2)
				return nullptr;
		}

		return newInfo;
	}

	BattlePassInfo::SharedPtr CBattlePassManager::InsertPassInfo(BattlePassInfo::SharedPtr newInfo)
	{
		if (newInfo == nullptr)
			return nullptr;

		if (SeekPassInfo(newInfo->PassKind) != nullptr)
			return nullptr;

		auto [iter_1, result_1] = repo_Pass.emplace(newInfo->PassKind, newInfo);
		if (!result_1)
			return nullptr;

		return newInfo;
	}

	BattlePassMissionInfo::SharedPtr CBattlePassManager::InsertMissionInfo(BattlePassMissionInfo::SharedPtr newInfo)
	{
		if (newInfo == nullptr)
			return nullptr;

		if (SeekMissionInfo(newInfo->PassKind, newInfo->Missionkind) != nullptr)
			return nullptr;

		auto iter1 = repo_Mission.find(newInfo->PassKind);
		if (iter1 != repo_Mission.end())
		{
			auto& missionMap = (*iter1).second;

			auto [iter_1, result_1] = missionMap.emplace(newInfo->Missionkind, newInfo);
			if (!result_1)
				return nullptr;
		}
		else
		{
			std::unordered_map<INT64, BattlePassMissionInfo::SharedPtr> missionMap;

			auto [iter_1, result_1] = missionMap.emplace(newInfo->Missionkind, newInfo);
			if (!result_1)
				return nullptr;

			auto [iter_2, result_2] = repo_Mission.emplace(newInfo->PassKind, missionMap);
			if (!result_2)
				return nullptr;
		}

		auto iter2 = repo_Type_Mission.find(newInfo->PassKind);
		if (iter2 != repo_Type_Mission.end())
		{
			iter2->second.emplace(newInfo->MissionScheduleType, newInfo);
		}
		else
		{
			repo_Type_Mission.emplace(newInfo->PassKind, std::unordered_multimap<ePassMissionType, BattlePassMissionInfo::WeakPtr>());
			repo_Type_Mission[newInfo->PassKind].emplace(newInfo->MissionScheduleType, newInfo);
		}
		
		return newInfo;
	}

	const BattlePassScheduleInfo::SharedPtr CBattlePassManager::GetSchedule(const INT32 passkind) const
	{
		return SeekSchedule(passkind);
	}

	const BattlePassRewardGroupInfo::SharedPtr CBattlePassManager::GetRewardGroup(const INT32 passkind, const INT32 groupid) const
	{
		return SeekRewardGroup(passkind, groupid);
	}

	const BattlePassPointinfo::SharedPtr CBattlePassManager::GetPointInfo(const INT32 passkind, const INT32 groupid, const INT32 tier) const
	{
		return SeekPointInfo(passkind, groupid, tier);
	}

	const BattlePassInfo::SharedPtr CBattlePassManager::GetPassInfo(const INT32 passkind) const
	{
		return SeekPassInfo(passkind);
	}

	const BattlePassInfo::SharedPtr CBattlePassManager::GetPassInfoByPackageKind(const INT32 packageKind) const
	{
		for (const auto& [passKind, pass] : repo_Pass)
		{
			auto passInfo = pass.get();
			if (passInfo == nullptr)
				continue;

			if (passInfo->PackageKind == packageKind)
				return pass;
		}

		return nullptr;
	}
	
	const BattlePassMissionInfo::SharedPtr CBattlePassManager::GetMission(const INT32 passkind, const INT64 missionkind) const
	{
		return SeekMissionInfo(passkind, missionkind);
	}

	auto CBattlePassManager::GetMissionInfos(const INT32 passkind)->std::unordered_map<INT64, BattlePassMissionInfo::SharedPtr>*
	{
		if (repo_Mission.end() == repo_Mission.find(passkind))
			return nullptr;

		return &(repo_Mission.at(passkind));
	}

	auto CBattlePassManager::GetMissionInfosByType(const INT32 passkind) -> std::unordered_multimap<ePassMissionType, BattlePassMissionInfo::WeakPtr>*
	{
		const auto missions = repo_Type_Mission.find(passkind);
		if (repo_Type_Mission.end() == missions)
			return nullptr;

		return &(missions->second);
	}


	INT64 CBattlePassManager::GetPassMaxPoint(const INT32 passkind, const INT32 rewardGroup)
	{
		INT64 resultMaxPoint = 0;
		
		//repo_Point
		auto iter1 = repo_Point.find(std::make_pair(passkind, rewardGroup));
		if (iter1 == repo_Point.end())
			return 0;

		const auto& pointMap = (*iter1).second;

		const auto maxPoint = pointMap.size() * BattlePass::TIER_TO_POINT;

		const INT64 INT64MaxValue = std::numeric_limits<INT64>::max();
		
		if (maxPoint > INT64MaxValue)
			resultMaxPoint = std::numeric_limits<INT64>::max();
		else
			resultMaxPoint = static_cast<INT64>(maxPoint);
		
		return resultMaxPoint;
	}

	bool CBattlePassManager::CheckPassBuffStat(const INT32 passKind, BuffContents::eBUFF_STAT_KIND _buffStatKind) const
	{
		bool hasBuffStat = false;

		auto passInfo = SeekPassInfo(passKind);
		if (passInfo == nullptr)
			return hasBuffStat;

		for (auto buffItemKind : passInfo->vecSeasonalBuff)
		{
			auto pItemInfo = BASE::GET_ITEM_DATA(buffItemKind);
			if (IS_NULL(pItemInfo))
				continue;

			auto buffInfo = BASE::BUFF_ELEM_DATA.Get(pItemInfo->i32VALUE[0]);
			if (buffInfo == nullptr)
				continue;

			if (buffInfo->ContainsKind(_buffStatKind))
			{
				hasBuffStat = true;
				break;
			}
		}

		return hasBuffStat;
	}

	const INT32 CBattlePassManager::GetRewardGroupIndex(const INT32 passkind, const INT64 curTime)
	{
		auto iter = repo_RewardGroup.find(passkind);
		if (iter == repo_RewardGroup.end())
			return 0;

		auto& rewardGroupMap = (*iter).second;
		auto serverAge = GetServerAge(curTime);


		for (const auto& [groupid, group]: rewardGroupMap)
		{
			auto groupInfo = group.get();
			if (groupInfo == nullptr)
				continue;

			if (groupInfo->ServerAgeMin <= serverAge && groupInfo->ServerAgeMax >= serverAge)
				return groupid;
		}

		return 0;
	}

	INT64 CBattlePassManager::GetNeedGem(const INT64 curPoint, const INT64 targetPoint)
	{
		// 이거 급해서 일단 100 단위로 쪼개서 한다.
		// 꼭! ndt 순회해서 진짜 티어당 필요한 포인트 계산해야한다.

		const INT64 curTier = curPoint / TIER_TO_POINT;
		const INT64 targetTier = targetPoint / TIER_TO_POINT;

		return (targetTier - curTier) * TIER_TO_POINT;
	}

	INT32 CBattlePassManager::GetUTCLastDayOfMonth(const INT64 curTime)
	{
		auto curYear = GetCurrentUTCYear(curTime) + START_YEAR;
		auto curMonth = GetCurrentUTCMonth(curTime) + 1;

		curYear = (curMonth == MAX_MONTH) ? (curYear + 1) : curYear;
		curMonth = (curMonth == MAX_MONTH) ? 1 : curMonth + 1;

		auto nextMonthUTCTime = GetUTCTime(curYear, curMonth, 1, 0);
		
		return nextMonthUTCTime;
	}

	bool CBattlePassManager::HasItemInVecGiveItem(INT32 passKind, INT32 itemKind)
	{
		const auto passScheduleInfo = GetSchedule(passKind);
		if (passScheduleInfo == nullptr)
			return false;

		for (const auto& item : passScheduleInfo->vecGiveItem)
		{
			if (item.Kind == itemKind)
			{
				return true;
			}
		}
		return false;
	}

	void CBattlePassManager::Send_DB_BATTLEPASS_LOAD_START()
	{
		auto& repo = CUserManager::Instance()->GetRepository();
		for (auto iter = repo.begin(); iter != repo.end(); ++iter)
		{
			CUser::SharedPtr pUser = iter->second;
			if (IS_NULL(pUser) || false == pUser->IsPC())
				continue;

			m_userIDs.push(pUser->UID());
		}

		Send_DB_BATTLEPASS_LOAD_REQ();
	}

	void CBattlePassManager::Send_DB_BATTLEPASS_LOAD_REQ()
	{
		INT32 cnt = 512;
		std::vector<INT64> userIDs;
		while (false == m_userIDs.empty() && cnt > 0)
		{
			INT64 userID = m_userIDs.top();
			m_userIDs.pop();
			userIDs.emplace_back(userID);
			cnt--;
		}
		CBattlePassHelper::P_BATTLEPASS_LOAD(userIDs, true);
	}

	INT8 CBattlePassManager::Recv_DB_BATTLEPASS_LOAD_ACK(const void* pData)
	{
		if (m_userIDs.empty() == false)
		{
			Send_DB_BATTLEPASS_LOAD_REQ();
			return GameWorkerLoader::STATE::UPDATE;
		}
		LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"GAMEWORKERLOADER : Recv_DB_BATTLEPASS_LOAD_ACK");
		return GameWorkerLoader::STATE::NEXT;
	}

	SRESULT CBattlePassManager::LoadSchedule(NDataReader& data)
	{
		repo_Schedule.clear();

		INT32 dataCount = 0;

		NDT_LOOP2(data, _T("Table"))
		{
			int index = 0;
			const NDataReader::Row& row = data.GetCurrentRow();

			auto pScheduleInfo = std::make_shared<BattlePassScheduleInfo>();
			if (!pScheduleInfo)
				throw std::bad_alloc();

			row.GetColumn(index++, pScheduleInfo->PassKind);
			index++;
			row.GetColumn(index++, pScheduleInfo->MinServerAge);
			row.GetColumn(index++, pScheduleInfo->ScheduleType);
			row.GetColumn(index++, pScheduleInfo->ScheduleValue);
			row.GetColumn(index++, pScheduleInfo->ProgressDay);
			row.GetColumn(index++, pScheduleInfo->Dayoff);
			row.GetColumn(index++, pScheduleInfo->MinLevel); 

			std::wstring strToken;
			std::vector<wstring> ItemInfos;
			std::vector<wstring> ItemInfo;
			ItemInfos.clear(); ItemInfo.clear();

			//패스 구입시 지급하는 조각 아이템
			{
				row.GetColumn(index++, strToken);
				GLOBAL::Tokenize(strToken, ItemInfos, _T("(;)"));
				for (int i = 0; i < ItemInfos.size(); ++i)
				{
					GLOBAL::Tokenize(ItemInfos[i], ItemInfo, _T(":"));

					if (ItemInfo.size() == 2)
					{
						auto itemKind = _ttoi(ItemInfo[0].c_str());
						auto itemCount = _ttoi(ItemInfo[1].c_str());

						pScheduleInfo->vecGiveItem.emplace_back(BASE::REWARDITEM(itemKind, itemCount));
					}
				}
			}

			BattlePass::CBattlePassManager::Instance()->InsertSchedule(pScheduleInfo);

			++dataCount;
		
		}
		NDT_LOOP_END;

		LOGGER_INFO(CONST_NDT_LOG, L"Load common_battlepass_schedule_info Completed (Count: {})", dataCount);

		return SERVER_OK;
	}

	SRESULT CBattlePassManager::LoadRewardGroup(NDataReader& data)
	{
		repo_RewardGroup.clear();

		INT32 dataCount = 0;

		NDT_LOOP2(data, _T("Table"))
		{
			int index = 0;
			const NDataReader::Row& row = data.GetCurrentRow();

			auto pRewardGroupInfo = std::make_shared<BattlePassRewardGroupInfo>();
			if (!pRewardGroupInfo)
				throw std::bad_alloc();

			row.GetColumn(index++, pRewardGroupInfo->PassKind);
			row.GetColumn(index++, pRewardGroupInfo->RewardGroup);
			row.GetColumn(index++, pRewardGroupInfo->ServerAgeMin);
			row.GetColumn(index++, pRewardGroupInfo->ServerAgeMax);

			BattlePass::CBattlePassManager::Instance()->InsertRewardGroup(pRewardGroupInfo);

			++dataCount;

		}
		NDT_LOOP_END;

		LOGGER_INFO(CONST_NDT_LOG, L"Load common_battlepass_reward_group_info Completed (Count: {})", dataCount);

		return SERVER_OK;
	}

	SRESULT CBattlePassManager::LoadPoint(NDataReader& data)
	{
		repo_Point.clear();

		INT32 dataCount = 0;

		NDT_LOOP2(data, _T("Table"))
		{
			int index = 0;
			const NDataReader::Row& row = data.GetCurrentRow();

			auto pPointInfo = std::make_shared<BattlePassPointinfo>();
			if (!pPointInfo)
				throw std::bad_alloc();

			row.GetColumn(index++, pPointInfo->PassKind);
			row.GetColumn(index++, pPointInfo->RewardGroup);
			row.GetColumn(index++, pPointInfo->PassTier);
			row.GetColumn(index++, pPointInfo->NeedPoint);
			row.GetColumn(index++, pPointInfo->NeedGem);
			row.GetColumn(index++, pPointInfo->PremiumReward);

			std::wstring strToken;
			std::vector<wstring> ItemInfos;
			std::vector<wstring> ItemInfo;
			ItemInfos.clear(); ItemInfo.clear();

			//무료 보상
			{
				row.GetColumn(index++, strToken);
				GLOBAL::Tokenize(strToken, ItemInfos, _T("(;)"));
				for (int i = 0; i < ItemInfos.size(); ++i)
				{
					GLOBAL::Tokenize(ItemInfos[i], ItemInfo, _T(":"));

					if (ItemInfo.size() == 2)
					{
						auto itemKind = _ttoi(ItemInfo[0].c_str());
						auto itemCount = _ttoi(ItemInfo[1].c_str());

						pPointInfo->vecFreeReward.emplace_back(BASE::REWARDITEM(itemKind, itemCount));
					}
				}
			}
			ItemInfos.clear(); ItemInfo.clear();
			// 유료보상
			{
				row.GetColumn(index++, strToken);
				GLOBAL::Tokenize(strToken, ItemInfos, _T("(;)"));
				for (int i = 0; i < ItemInfos.size(); ++i)
				{
					GLOBAL::Tokenize(ItemInfos[i], ItemInfo, _T(":"));

					if (ItemInfo.size() == 2)
					{
						auto itemKind = _ttoi(ItemInfo[0].c_str());
						auto itemCount = _ttoi(ItemInfo[1].c_str());

						pPointInfo->vecPaidReward.emplace_back(BASE::REWARDITEM(itemKind, itemCount));
					}
				}
			}
			row.GetColumn(index++, pPointInfo->NotiFlag);

			BattlePass::CBattlePassManager::Instance()->InsertPointInfo(pPointInfo);

			++dataCount;
		}
		NDT_LOOP_END;

		LOGGER_INFO(CONST_NDT_LOG, L"Load common_battlepass_point_info Completed (Count: {})", dataCount);

		return SERVER_OK;
	}

	SRESULT CBattlePassManager::LoadPass(NDataReader& data)
	{
		repo_Pass.clear();

		INT32 dataCount = 0;

		NDT_LOOP2(data, _T("Table"))
		{
			int index = 0;
			const NDataReader::Row& row = data.GetCurrentRow();

			auto pPassInfo = std::make_shared<BattlePassInfo>();
			if (!pPassInfo)
				throw std::bad_alloc();

			row.GetColumn(index++, pPassInfo->PassKind);
			row.GetColumn(index++, pPassInfo->NeedGem);
			row.GetColumn(index++, pPassInfo->SaleRate);
			row.GetColumn(index++, pPassInfo->TierUpto);

			std::wstring strToken;
			std::vector<wstring> Tokens, Tokens2;

			{
				Tokens.clear(); Tokens2.clear();
				row.GetColumn(index++, strToken);
				GLOBAL::Tokenize(strToken, Tokens, _T(";"));
				for (auto& token : Tokens)
				{
					pPassInfo->vecSeasonalBuff.push_back(_ttoi(token.c_str()));
				}
			}

			{
				row.GetColumn(index++, strToken);
				Tokens.clear(); Tokens2.clear();
				GLOBAL::Tokenize(strToken, Tokens, _T(";"));
				for (auto& token : Tokens)
				{
					GLOBAL::Tokenize(token, Tokens2, _T("(:)"));
					if (Tokens2.size() == 2)
					{
						pPassInfo->vecGuildPresent.push_back(BASE::REWARDITEM(_ttoi(Tokens2[0].c_str()), _ttoi(Tokens2[1].c_str())));
					}
				}
			}

			{
				row.GetColumn(index++, strToken);
				Tokens.clear(); Tokens2.clear();
				GLOBAL::Tokenize(strToken, Tokens, _T(";"));
				for (auto& token : Tokens)
				{
					GLOBAL::Tokenize(token, Tokens2, _T("(:)"));
					if (Tokens2.size() == 2)
					{
						pPassInfo->vecSpecialBenefit.push_back(BASE::REWARDITEM(_ttoi(Tokens2[0].c_str()), _ttoi(Tokens2[1].c_str())));
					}
				}
			}

			row.GetColumn(index++, pPassInfo->PackageGroupCode);
			row.GetColumn(index++, pPassInfo->PackageKind);

			BattlePass::CBattlePassManager::Instance()->InsertPassInfo(pPassInfo);

			++dataCount;
		}
		NDT_LOOP_END;

		LOGGER_INFO(CONST_NDT_LOG, L"Load common_battlepass_pass_info Completed (Count: {})", dataCount);

		return SERVER_OK;
	}

	SRESULT CBattlePassManager::LoadMission(NDataReader& data)
	{
		repo_Mission.clear();
		repo_Type_Mission.clear();

		INT32 dataCount = 0;

		NDT_LOOP2(data, _T("Table"))
		{
			int index = 0;
			const NDataReader::Row& row = data.GetCurrentRow();

			auto pMissionInfo = std::make_shared<BattlePassMissionInfo>();
			if (!pMissionInfo)
				throw std::bad_alloc();

			row.GetColumn(index++, pMissionInfo->PassKind);
			row.GetColumn(index++, pMissionInfo->Missionkind);
			row.GetColumn(index++, pMissionInfo->MissionScheduleType);
			row.GetColumn(index++, pMissionInfo->MissionScheduleValue);
			row.GetColumn(index++, pMissionInfo->MissionPassType);
			row.GetColumn(index++, pMissionInfo->CastleLevelMin);
			row.GetColumn(index++, pMissionInfo->CastleLevelMax);
			row.GetColumn(index++, pMissionInfo->MissionGroup);
			row.GetColumn(index++, pMissionInfo->ConditionChronicle);
			row.GetColumn(index++, pMissionInfo->ConditionKind);
			row.GetColumn(index++, pMissionInfo->ConditionValue);
			row.GetColumn(index++, pMissionInfo->TargetValue);
			row.GetColumn(index++, pMissionInfo->RewardPoint);

			std::wstring strToken;
			std::vector<wstring> Tokens, Tokens2;

			row.GetColumn(index++, strToken);
			GLOBAL::Tokenize(strToken, Tokens, _T(";"));	// 세미콜론 단위로 나누고
			for (auto& tmp : Tokens)
			{
				GLOBAL::Tokenize(tmp, Tokens2, _T("(:)"));	// 카인드/갯수
				if (Tokens2.size() >= 2)
				{
					pMissionInfo->vecRewardItems.emplace_back(BASE::REWARDITEM(_ttoi(Tokens2[0].c_str()), _ttoi(Tokens2[1].c_str())));
				}
			}

			BattlePass::CBattlePassManager::Instance()->InsertMissionInfo(pMissionInfo);
			++dataCount;
		}
		NDT_LOOP_END;

		LOGGER_INFO(CONST_NDT_LOG, L"Load common_battlepass_mission_info Completed (Count: {})", dataCount);

		return SERVER_OK;
	}

}


