#include "GameAfx.h"

#include "_DatabaseThreadManager/DBQUERY/WebDB.h"
#include "_DatabaseThreadManager/DBAGlobal.h"
#include "_DatabaseThreadManager/DBQUERY/WDB/P_CUSTOM_EVENT_INFO_LOAD.h"
#include "_DatabaseThreadManager/DBQUERY/WDB/P_CUSTOM_EVENT_TEXT_INFO_LOAD.h"

namespace CustomEvent {

void CCustomEventInfoManager::Clear()
{
	repo_Schedule.clear();
	repo_Round.clear();
	repo_Reward.clear();
	repo_Probability.clear();
	repo_DeleteItem.clear();
	repo_Quest.clear();
	
	map_CurSchedule.clear();
}

const ScheduleInfo::SharedPtr CCustomEventInfoManager::SeekSchedule(const INT64 eventID) const
{
	auto iterF = repo_Schedule.find(eventID);
	if (iterF == repo_Schedule.end())
		return nullptr;

	return (*iterF).second;
}

const RoundInfo::SharedPtr CCustomEventInfoManager::SeekRound(const INT64 eventID, const INT32 round) const
{
	auto iter1 = repo_Round.find(eventID);
	if (iter1 == repo_Round.end())
		return nullptr;

	auto& roundMap = (*iter1).second;
	auto iter2 = roundMap.find(round);
	if (iter2 == roundMap.end())
		return nullptr;

	return (*iter2).second;
}

const RewardInfo::SharedPtr CCustomEventInfoManager::SeekReward(const INT32 groupID, const GAME::eNATION nation, const INT32 rewardType) const
{
	auto iter1 = repo_Reward.find(groupID);
	if (iter1 == repo_Reward.end())
		return nullptr;

	auto& rewardMap2 = (*iter1).second;
	auto iter2 = rewardMap2.find(nation);
	if (iter2 == rewardMap2.end())
		return nullptr;

	auto& rewardMap3 = (*iter2).second;
	auto iter3 = rewardMap3.find(rewardType);
	if (iter3 == rewardMap3.end())
		return nullptr;

	return (*iter3).second;
}

const ProbabilityInfo::SharedPtr CCustomEventInfoManager::SeekProbability(const INT64 eventID, const std::tuple<INT32, INT32> key) const
{
	auto iter1 = repo_Probability.find(eventID);
	if (iter1 == repo_Probability.end())
		return nullptr;

	auto& ProbabilityMap = (*iter1).second;
	auto iter2 = ProbabilityMap.find(key);
	if (iter2 == ProbabilityMap.end())
		return nullptr;

	return (*iter2).second;
}

const DeleteItemInfo::SharedPtr CCustomEventInfoManager::SeekDeleteItemInfo(const INT64 eventID) const
{
	auto iterF = repo_DeleteItem.find(eventID);
	if (iterF == repo_DeleteItem.end())
		return nullptr;

	return (*iterF).second;
}

const QuestInfo::SharedPtr CCustomEventInfoManager::SeekQuest(const INT64 groupID, const INT64 questID) const
{
	const auto iter1 = repo_Quest.find(groupID);
	if (iter1 == repo_Quest.end())
		return nullptr;

	auto& questMap = (*iter1).second;

	auto iter2 = questMap.find(questID);
	if (iter2 == questMap.end())
		return nullptr;

	return (*iter2).second;
}

const ValueInfo::SharedPtr CCustomEventInfoManager::SeekValue(const INT64 eventID, const eCustomEventDifficulty difficulty) const
{
	auto iter1 = repo_ValueInfo.find(eventID);
	if (iter1 == repo_ValueInfo.end())
		return nullptr;

	auto& valueMap = (*iter1).second;

	auto iter2 = valueMap.find(difficulty);
	if (iter2 == valueMap.end())
		return nullptr;

	return (*iter2).second;
}
	
ScheduleInfo::SharedPtr CCustomEventInfoManager::InsertSchedule(ScheduleInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	if (SeekSchedule(newInfo->EventID) != nullptr)
		return nullptr;

	auto [iter_1, result_1] = repo_Schedule.emplace(newInfo->EventID, newInfo);
	if (!result_1)
		return nullptr;

	return newInfo;
}

RoundInfo::SharedPtr CCustomEventInfoManager::InsertRound(RoundInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	if (SeekRound(newInfo->EventID, newInfo->Round) != nullptr)
		return nullptr;

	auto iter1 = repo_Round.find(newInfo->EventID);
	if (iter1 != repo_Round.end())
	{
		auto& roundMap = (*iter1).second;
		auto [iter_1, result_1] = roundMap.emplace(newInfo->Round, newInfo);
		if (!result_1)
			return nullptr;
	}
	else
	{
		std::unordered_map<INT32, RoundInfo::SharedPtr> roundMap;
		auto [iter_1, result_1] = roundMap.emplace(newInfo->Round, newInfo);
		if (!result_1)
			return nullptr;

		auto [iter_2, result_2] = repo_Round.emplace(newInfo->EventID, roundMap);
		if (!result_2)
			return nullptr;
	}

	return newInfo;
}

RewardInfo::SharedPtr CCustomEventInfoManager::InsertReward(RewardInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	if (SeekReward(newInfo->RewardGroupID, newInfo->Nation, newInfo->RewardType) != nullptr)
		return nullptr;

	auto iter_GroupID = repo_Reward.find(newInfo->RewardGroupID);
	if (iter_GroupID != repo_Reward.end())
	{
		auto& rewardNationMap = (*iter_GroupID).second;

		auto iter_Nation = rewardNationMap.find(newInfo->Nation);
		if (iter_Nation != rewardNationMap.end())
		{
			auto& rewardTypeMap = (*iter_Nation).second;
			auto [iter_1, result_1] = rewardTypeMap.emplace(newInfo->RewardType, newInfo);
			if (!result_1)
				return nullptr;
		}
		else
		{
			std::unordered_map<INT32, RewardInfo::SharedPtr> rewardTypeMap;
			auto [iter_1, result_1] = rewardTypeMap.emplace(newInfo->RewardType, newInfo);
			if (!result_1)
				return nullptr;

			auto [iter_2, result_2] = rewardNationMap.emplace(newInfo->Nation, rewardTypeMap);
			if (!result_2)
				return nullptr;
		}
	}
	else
	{
		std::unordered_map<INT32, RewardInfo::SharedPtr> rewardTypeMap;
		auto [iter_1, result_1] = rewardTypeMap.emplace(newInfo->RewardType, newInfo);
		if (!result_1)
			return nullptr;

		std::unordered_map<GAME::eNATION, std::unordered_map<INT32, RewardInfo::SharedPtr>> rewardNationMap;
		auto [iter_2, result_2] = rewardNationMap.emplace(newInfo->Nation, rewardTypeMap);
		if (!result_2)
			return nullptr;

		auto [iter_3, result_3] = repo_Reward.emplace(newInfo->RewardGroupID, rewardNationMap);
		if (!result_3)
			return nullptr;
	}

	return newInfo;
}

ProbabilityInfo::SharedPtr CCustomEventInfoManager::InsertProbability(ProbabilityInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	std::tuple<INT32, INT32> Key = std::make_tuple(newInfo->ProbKey_1, newInfo->ProbKey_2);
	if (SeekProbability(newInfo->EventID, Key) != nullptr)
		return nullptr;

	auto iter1 = repo_Probability.find(newInfo->EventID);
	if (iter1 != repo_Probability.end())
	{
		auto& ProbabilityMap = (*iter1).second;
		auto [iter_1, result_1] = ProbabilityMap.emplace(Key, newInfo);
		if (!result_1)
			return nullptr;
	}
	else
	{
		std::unordered_map<std::tuple<INT32, INT32>, ProbabilityInfo::SharedPtr> ProbabilityMap;
		auto [iter_1, result_1] = ProbabilityMap.emplace(Key, newInfo);
		if (!result_1)
			return nullptr;

		auto [iter_2, result_2] = repo_Probability.emplace(newInfo->EventID, ProbabilityMap);
		if (!result_2)
			return nullptr;
	}

	return newInfo;
}

TextInfo::SharedPtr CCustomEventInfoManager::InsertText(GAME::eLANGUAGE_CODE langCode, std::wstring textKey, std::wstring textValue)
{
	TextInfo::SharedPtr newInfo;

	auto iter1 = repo_Text.find(langCode);
	if (iter1 != repo_Text.end())
	{
		newInfo = (*iter1).second;
		auto iter2 = newInfo->LanguageMap.find(textKey);
		if (iter2 != newInfo->LanguageMap.end())
		{
			newInfo->LanguageMap[textKey] = textValue;
		}
		else
		{
			newInfo->LanguageMap.emplace(textKey, textValue);
		}
	}
	else
	{
		newInfo = std::make_shared<TextInfo>();
		if (!newInfo)
			throw std::bad_alloc();

		newInfo->LanguageMap.emplace(textKey, textValue);
		repo_Text.emplace(langCode, newInfo);
	}

	return newInfo;
}

DeleteItemInfo::SharedPtr CCustomEventInfoManager::InsertDeleteItem(DeleteItemInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	auto deleteItemInfo = SeekDeleteItemInfo(newInfo->EventID);

	if (deleteItemInfo != nullptr)
	{
		repo_DeleteItem[newInfo->EventID] = newInfo;
	}
	else
	{
		auto [iter_1, result_1] = repo_DeleteItem.emplace(newInfo->EventID, newInfo);
		if (!result_1)
			return nullptr;
	}

	return newInfo;
}

QuestInfo::SharedPtr CCustomEventInfoManager::InsertQuest(QuestInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	if (SeekQuest(newInfo->GroupID, newInfo->QuestID) != nullptr)
		return nullptr;

	auto iterGroup = repo_Quest.find(newInfo->GroupID);
	if (iterGroup != repo_Quest.end())
	{
		auto& questMap = (*iterGroup).second;

		auto [iter_1, result_1] = questMap.emplace(newInfo->QuestID, newInfo);
		if (result_1 == false)
			return nullptr;
	}
	else
	{
		std::unordered_map<INT64, QuestInfo::SharedPtr> questMap;

		auto [iter_1, result_1] = questMap.emplace(newInfo->QuestID, newInfo);
		if (result_1 == false)
			return nullptr;

		auto [iter_2, result_2] = repo_Quest.emplace(newInfo->GroupID, questMap);
		if (!result_2)
			return nullptr;
	}

	
	return newInfo;
}

ValueInfo::SharedPtr CCustomEventInfoManager::InsertValue(ValueInfo::SharedPtr newInfo)
{
	if (!newInfo)
		return nullptr;

	if (SeekValue(newInfo->EventID, static_cast<eCustomEventDifficulty>(newInfo->Difficulty)) != nullptr)
		return nullptr;

	auto iterEventValue = repo_ValueInfo.find(newInfo->EventID);
	if (iterEventValue != repo_ValueInfo.end())
	{
		auto& valueMap = (*iterEventValue).second;

		auto [iter_1, result_1] = valueMap.emplace(static_cast<eCustomEventDifficulty>(newInfo->Difficulty), newInfo);
		if (result_1 == false)
			return nullptr;
	}
	else
	{
		std::unordered_map<eCustomEventDifficulty, ValueInfo::SharedPtr> valueMap;

		auto [iter_1, result_1] = valueMap.emplace(static_cast<eCustomEventDifficulty>(newInfo->Difficulty), newInfo);
		if (result_1 == false)
			return nullptr;

		auto [iter_2, result_2] = repo_ValueInfo.emplace(newInfo->EventID, valueMap);
		if (result_2 == false)
			return nullptr;
	}

	return newInfo;
}

const ScheduleInfo::SharedPtr CCustomEventInfoManager::GetScheduleInfo(const INT64 eventID) const
{
	return SeekSchedule(eventID);
}

const RoundInfo::SharedPtr CCustomEventInfoManager::GetRoundInfo(const INT64 eventID, const INT32 round) const
{
	return SeekRound(eventID, round);
}

const RewardInfo::SharedPtr CCustomEventInfoManager::GetRewardInfo(const INT32 groupID, const GAME::eNATION nation, const INT32 rewardType) const
{
	return SeekReward(groupID, nation, rewardType);
}

const ProbabilityInfo::SharedPtr CCustomEventInfoManager::GetProbabilityInfo(const INT64 eventID, const INT64 mainKey, const INT64 subKey) const
{
	return SeekProbability(eventID, std::make_tuple(mainKey, subKey));
}

const DeleteItemInfo::SharedPtr CCustomEventInfoManager::GetDeleteItemInfo(const INT64 eventID) const
{
	return SeekDeleteItemInfo(eventID);
}

const QuestInfo::SharedPtr CCustomEventInfoManager::GetQuestInfo(const INT64 eventID, const INT64 questID) const
{
	ScheduleInfo::SharedPtr scheduleInfo = SeekSchedule(eventID);
	if (scheduleInfo == nullptr)
		return nullptr;

	return SeekQuest(scheduleInfo->QuestGroup, questID);
}

const ValueInfo::SharedPtr CCustomEventInfoManager::GetValueInfo(const INT64 eventID, const eCustomEventDifficulty difficulty) const
{
	return SeekValue(eventID, difficulty);
}

const TextInfo::SharedPtr CCustomEventInfoManager::GetTextInfo(const GAME::eLANGUAGE_CODE langCode) const
{
	auto iterF = repo_Text.find(langCode);
	if (iterF == repo_Text.end())
		return nullptr;

	return (*iterF).second;
}

void CCustomEventInfoManager::GetRoundInfos(const INT64 eventID, std::vector<RoundInfo::SharedPtr>& out_vecRoundInfos)
{
	auto iter_eventID = repo_Round.find(eventID);
	if (iter_eventID == repo_Round.end())
		return;

	auto& roundMap = (*iter_eventID).second;
	for (const auto& pair_round : roundMap)
	{
		out_vecRoundInfos.emplace_back(pair_round.second);
	}

	return;
}

void CCustomEventInfoManager::GetCalendarRewardInfos(const INT64 eventID, const INT32 round, const GAME::eNATION nation, OUT std::vector<std::tuple<INT32, RewardInfo::SharedPtr>>& out_vecRewardInfos)
{
	auto iter_eventID = repo_Round.find(eventID);
	if (iter_eventID == repo_Round.end())
		return;

	std::vector<INT32> vec_Key;

	auto& roundMap = (*iter_eventID).second;
	for (const auto& pair_round : roundMap)
		vec_Key.emplace_back(pair_round.first);

	std::sort(vec_Key.begin(), vec_Key.end());

	auto roundIter = roundMap.find(vec_Key.back());
	if (roundIter != roundMap.end())
	{
		auto iter_rewardGroup = repo_Reward.find((*roundIter).second->RoundRewardGroupID);
		if (iter_rewardGroup != repo_Reward.end())
		{
			auto& rewardNationMap = (*iter_rewardGroup).second;

			auto iter_Nation = rewardNationMap.find(GAME::eNATION::eNATION_NONE);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple((*roundIter).first, pair_type.second));
			}

			iter_Nation = rewardNationMap.find(nation);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple((*roundIter).first, pair_type.second));
			}
		}
	}

	return;
}


void CCustomEventInfoManager::GetRewardInfos(const INT64 eventID, const INT32 round, const GAME::eNATION nation, OUT std::vector<std::tuple<INT32, RewardInfo::SharedPtr>>& out_vecRewardInfos)
{
	auto iter_eventID = repo_Round.find(eventID);
	if (iter_eventID == repo_Round.end())
		return;

	auto& roundMap = (*iter_eventID).second;
	for (const auto& pair_round : roundMap)
	{
		if (0 < round && round != pair_round.first)
			continue;

		auto iter_RewardGroup = repo_Reward.find(pair_round.second->RoundRewardGroupID);
		if (iter_RewardGroup != repo_Reward.end())
		{
			auto& rewardNationMap = (*iter_RewardGroup).second;

			auto iter_Nation = rewardNationMap.find(GAME::eNATION::eNATION_NONE);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple(pair_round.first, pair_type.second));
			}

			iter_Nation = rewardNationMap.find(nation);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple(pair_round.first, pair_type.second));
			}
		}

		iter_RewardGroup = repo_Reward.find(pair_round.second->FixRewardGroupID);
		if (iter_RewardGroup != repo_Reward.end())
		{
			auto& rewardNationMap = (*iter_RewardGroup).second;

			auto iter_Nation = rewardNationMap.find(GAME::eNATION::eNATION_NONE);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple(pair_round.first, pair_type.second));
			}

			iter_Nation = rewardNationMap.find(nation);
			if (iter_Nation != rewardNationMap.end())
			{
				auto& rewardTypeMap = (*iter_Nation).second;

				for (const auto& pair_type : rewardTypeMap)
					out_vecRewardInfos.emplace_back(std::make_tuple(pair_round.first, pair_type.second));
			}
		}
	}

	return;
}

void CCustomEventInfoManager::GetProbabilityInfos(const INT64 eventID, std::vector<ProbabilityInfo::SharedPtr>& out_vecProbabilityInfos)
{
	auto iter_eventID = repo_Probability.find(eventID);
	if (iter_eventID == repo_Probability.end())
		return;

	auto& probabilityMap = (*iter_eventID).second;
	for (const auto& pair_probability : probabilityMap)
	{
		out_vecProbabilityInfos.emplace_back(pair_probability.second);
	}

	return;
}

auto CCustomEventInfoManager::GetValueInfos(const INT64 eventID)->std::unordered_map<eCustomEventDifficulty, ValueInfo::SharedPtr>*
{
	if (repo_ValueInfo.find(eventID) == repo_ValueInfo.end())
		return nullptr;

	return &(repo_ValueInfo.at(eventID));
}

INT64 CCustomEventInfoManager::GetEventIDByKeyItem(const INT32 keyItemKind)
{
	INT64 result = 0;

	for (const auto& pair : repo_Schedule)
	{
		ScheduleInfo::SharedPtr schedule = pair.second;
		if (schedule == nullptr)
			continue;

		if (schedule->KeyItemKind == keyItemKind)
		{
			result = schedule->EventID;
			break;
		}
	}

	return result;
}

auto CCustomEventInfoManager::GetQuestInfos(const INT64 groupID)->std::unordered_map<INT64, QuestInfo::SharedPtr>*
{
	if (repo_Quest.find(groupID) == repo_Quest.end())
		return nullptr;

	return &(repo_Quest.at(groupID));
}
	
void CCustomEventInfoManager::Update()
{
	INT64 curTime = GetDueDay_UTC(0);

	if (false == CheckSameHour_UTC(m_lUpdateTime, curTime))
	{
		RefreshCurrentEventInfo(curTime);
		if (false == IsReady())
			SetDirtyAllUser();
		m_lUpdateTime = curTime;
	}
}

const bool CCustomEventInfoManager::IsReady() const
{
	return (m_lUpdateTime > 0LL);
}

bool CCustomEventInfoManager::RefreshCurrentEventInfo(const INT64 curTime)
{
	map_CurSchedule.clear();

	for (const auto& pair : repo_Schedule)
	{
		ScheduleInfo::SharedPtr schedule = pair.second;
		if (schedule == nullptr)
			continue;

		if (schedule->Active == 0)
			continue;

		if (schedule->EventType <= eEventType::NONE || eEventType::END <= schedule->EventType)
			continue;

		if (false == schedule->TargetServer)
		{
			continue;
		}

		if (schedule->BanServer)
		{
			continue;
		}

		INT64 serverCreateTime = EventHelper::GetAdjustTime(GetServerCreateTime());
		INT64 startTime = 0;
		INT64 endTime = 0;
		INT64 duration = std::max(schedule->EventDuration, schedule->RewardDuration);

		if (schedule->ScheduleType == eScheduleType::UTC_DATE)
		{
			startTime = schedule->ScheduleValue;
			endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);
		}
		else if (schedule->ScheduleType == eScheduleType::SERVER_AGE)
		{
			startTime = serverCreateTime + (static_cast<INT64>(SECOND_PER_DAY) * (schedule->ScheduleValue - 1));
			endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);
		}
		else if (schedule->ScheduleType == eScheduleType::USER_AGE)
		{
			// 유저 생성주기 스케줄은 유저 컨테이너에서 계산한다
			map_CurSchedule.emplace(schedule->EventID, schedule);
			continue;
		}

		if (startTime < 0 || endTime < 0 || endTime < curTime)
			continue;

		// 서버 나이 필터
		if (schedule->Min_ServerAge > 0 || schedule->Max_ServerAge > 0)
		{
			INT32 startServerAge = EventHelper::GetAdjustDay(serverCreateTime, startTime);
			INT32 endServerAge = EventHelper::GetAdjustDay(serverCreateTime, endTime);
			if (startServerAge < schedule->Min_ServerAge || schedule->Max_ServerAge < endServerAge)
				continue;
		}

		map_CurSchedule.emplace(schedule->EventID, schedule);
	}

	return (map_CurSchedule.size() > 0);
}

void CCustomEventInfoManager::SetDirty()
{
	m_lUpdateTime = -1LL;
}

void CCustomEventInfoManager::SetDirtyAllUser()
{
	// 전체 유저 SetDirty
	auto& repo = CUserManager::Instance()->GetRepository();
	for (auto iter = repo.begin(); iter != repo.end(); ++iter)
	{
		CUser::SharedPtr pUser = iter->second;
		if (IS_NULL(pUser) || false == pUser->IsPC())
			continue;

		ASE_INSTANCE(pUser, CCustomEventContainer)->SetDirty();
	}

	// 접속 중인 유저는 notify 까지
	for (auto itr = CSessionInfoManager::Instance()->GetRepository().begin(); itr != CSessionInfoManager::Instance()->GetRepository().end(); ++itr)
	{
		CUser* const pUser = CSessionInfoManager::GetUser(itr);
		if (nullptr == pUser || false == pUser->IsPC())
			continue;

		if (false == pUser->IsActive())
			continue;

		CustomEventHelper::CheckRefreshTime(pUser, false);
		CustomEventHelper::Send_GS_CUSTOM_EVENT_INFO_NFY(pUser);
	}
}

INT8 CCustomEventInfoManager::GetTimingGameTotalCount(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	//게임 횟수는 ValueInfo의 4번째 값..

	return valueInfo->Value4;
}

INT32 CCustomEventInfoManager::GetTimingGameSuccessBarSize(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value6;
}

INT32 CCustomEventInfoManager::GetTimingGameGreatSuccessBarSize(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value7;
}

INT32 CCustomEventInfoManager::GetTimingGameFailAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value1;
}

INT32 CCustomEventInfoManager::GetTimingGameSuccessAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value2;
}

INT32 CCustomEventInfoManager::GetTimingGameGreatSuccessAddItemCount(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value3;
}

INT32 CCustomEventInfoManager::GetWarDiceMagnification(const INT64 eventID, const INT32 round, const INT32 block)
{
	INT32 result = 1;

	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return result;

	const auto diceValueInfo = SeekValue(eventID, eCustomEventDifficulty::NONE);
	if (diceValueInfo == nullptr)
		return result;

	const auto roundInfo = SeekRound(eventID, round);
	if (roundInfo == nullptr)
		return result;

	if (schedule->EventType != eEventType::WARDICE)
		return result;

	eWARDICE_CITY_TYPE cityType = eWARDICE_CITY_TYPE::NONE;

	if (block == diceValueInfo->Value1)
		cityType = eWARDICE_CITY_TYPE::PARIS;
	else if (block == diceValueInfo->Value2)
		cityType = eWARDICE_CITY_TYPE::LONDON;
	else if (block == diceValueInfo->Value3)
		cityType = eWARDICE_CITY_TYPE::BERLIN;
	else if (block == diceValueInfo->Value4)
		cityType = eWARDICE_CITY_TYPE::MOSKVA;
	else if (block == diceValueInfo->Value5)
		cityType = eWARDICE_CITY_TYPE::ROMA;

	if (cityType == eWARDICE_CITY_TYPE::NONE)
		return result;

	auto index = static_cast<size_t>(cityType);
	if (index > roundInfo->RoundGoal.size())
		return result;

	result = roundInfo->RoundGoal[index - 1];

	return result;
}

INT32 CCustomEventInfoManager::GetTimingGameKeyItemKind(const INT64 eventID, const eCustomEventDifficulty difficulty)
{
	const auto schedule = SeekSchedule(eventID);
	if (schedule == nullptr)
		return 0;

	const auto valueInfo = SeekValue(eventID, difficulty);
	if (valueInfo == nullptr)
		return 0;

	if (schedule->EventType != eEventType::TIMINGGAME)
		return 0;

	return valueInfo->Value10;
}

void CCustomEventInfoManager::GetEventCalendarInfo(const INT64 calendarStartTime, const INT64 calendarEndTime, CUser* const pUser, OUT std::vector<EventCalendarData>& outInfo)
{
	if (IS_ACTIVE_USER(pUser) == false)
		return;

	auto languageType = pUser->GetLanguageType();
	auto textInfo = GetTextInfo((GAME::eLANGUAGE_CODE)languageType);
	if (textInfo == nullptr)
		return;

	INT64 serverCreateTime = EventHelper::GetAdjustTime(GetServerCreateTime());
	INT32 userCreateTime = EventHelper::GetAdjustTime(pUser->GetCreateTime());
	INT32 commandLv = pUser->Territory().GetBuildLevelFromKind(GAME::KIND_BUILD_COMMAND);
	INT32 payingLv = pUser->Get_PayingLv();

	auto& eventInfoMap = GetCurEventMap();

	for (const auto& [eventID, eventData] : eventInfoMap)
	{
		auto pEvent = eventData.lock();
		if (pEvent == nullptr)
			continue;

		if (pEvent->ColorIndex == 0)
			continue;

		ScheduleInfo::SharedPtr scheduleInfo = SeekSchedule(eventID);
		if (scheduleInfo == nullptr)
			continue;

		auto roundMap = repo_Round.find(eventID);
		if (roundMap == repo_Round.end())
			continue;
	
		std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
		GetCalendarRewardInfos(eventID, (*roundMap).second.size(), pUser->GetNation(), OUT out_vecReward);

		if (pEvent->Min_Level > 0 || pEvent->Max_Level > 0)
		{
			if (commandLv < pEvent->Min_Level || pEvent->Max_Level < commandLv)
				continue;
		}

		// 페잉 레벨 필터
		if (pEvent->Min_PayingLevel > 0 || pEvent->Max_PayingLevel > 0)
		{
			if (payingLv < pEvent->Min_PayingLevel || pEvent->Max_PayingLevel < payingLv)
				continue;
		}

		INT64 startTime = 0;
		INT64 endTime = 0;
		INT64 duration = std::max(pEvent->EventDuration, pEvent->RewardDuration);

		if (pEvent->ScheduleType == CustomEvent::eScheduleType::UTC_DATE)
		{
			startTime = pEvent->ScheduleValue;
		}
		else if (pEvent->ScheduleType == CustomEvent::eScheduleType::SERVER_AGE)
		{
			startTime = serverCreateTime + (static_cast<INT64>(SECOND_PER_DAY) * (pEvent->ScheduleValue - 1));
		}
		else if (pEvent->ScheduleType == CustomEvent::eScheduleType::USER_AGE)
		{
			startTime = userCreateTime + (static_cast<INT64>(SECOND_PER_DAY) * (pEvent->ScheduleValue - 1));
		}

		endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);

		if (startTime < calendarEndTime && endTime > calendarStartTime)
		{
			EventCalendarData calendaerData;

			auto titleText = textInfo->LanguageMap.find(scheduleInfo->IconKey);
			if (titleText != textInfo->LanguageMap.end())
				calendaerData.title = titleText->second;
			else
				continue;

			calendaerData.eventID = pEvent->EventID;
			calendaerData.startTime = startTime;
			calendaerData.endTime = endTime;
			calendaerData.colorIndex = pEvent->ColorIndex;
			calendaerData.titleTextKey = false;
			calendaerData.iconSprite = scheduleInfo->IconSprite;
			calendaerData.iconAtlas = scheduleInfo->IconAtlas;
			calendaerData.eventType = eCalendarEvent::CustomEvent;

			for (const auto& reward : out_vecReward)
			{
				if (calendaerData.rewardItems.size() >= EventCalendarHelper::MAX_REWARD_ITEM_CNT)
					break;

				auto pRewardInfo = (std::get<1>(reward)).get();
				if (pRewardInfo == nullptr)
					continue;

				for (const auto& item : pRewardInfo->RewardItems)
				{
					if (calendaerData.rewardItems.size() >= EventCalendarHelper::MAX_REWARD_ITEM_CNT)
						break;

					calendaerData.rewardItems.emplace_back(item);
				}
			}

			outInfo.emplace_back(calendaerData);
		}
	}
}

void CCustomEventInfoManager::P_CUSTOM_EVENT_INFO_LOAD(bool isServerStart)
{
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_WDB_QUERY_AUTO(P_CUSTOM_EVENT_INFO_LOAD);
			RUN_WDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, repo_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2(), repo_3 = pQuery->GetSET_3(),
                                      repo_4 = pQuery->GetSET_4(), repo_5 = pQuery->GetSET_5(), repo_6 = pQuery->GetSET_6(),
									  repo_7 = pQuery->GetSET_7(), repo_8 = pQuery->GetSET_8()]()->RECV_RESULT
			{
				CCustomEventInfoManager::Instance()->Clear();

				for (const auto& row_1 : repo_1)
				{
					auto pScheduleInfo = std::make_shared<ScheduleInfo>();
					if (pScheduleInfo == nullptr)
						throw std::bad_alloc();

					pScheduleInfo->EventID			= row_1.m_nEventID;
					pScheduleInfo->Active			= row_1.m_nActive;
					pScheduleInfo->TapInfo			= row_1.m_nTapInfo;
					pScheduleInfo->ColorIndex	    = row_1.m_nColorIndex;
					pScheduleInfo->EventType        = static_cast<eEventType>(row_1.m_nEventType);
					pScheduleInfo->ScheduleType		= static_cast<eScheduleType>(row_1.m_nScheduleType);
					pScheduleInfo->ScheduleValue	= row_1.m_nScheduleValue;
					pScheduleInfo->EventDuration	= row_1.m_nEventDuration;
					pScheduleInfo->RewardDuration	= row_1.m_nRewardDuration;
					pScheduleInfo->QuestGroup		= row_1.m_nQuestGroup;
					pScheduleInfo->Min_ServerAge	= row_1.m_nMinServerAge;
					pScheduleInfo->Max_ServerAge	= row_1.m_nMaxServerAge;
					pScheduleInfo->Min_UserAge		= row_1.m_nMinUserAge;
					pScheduleInfo->Max_UserAge		= row_1.m_nMaxUserAge;
					pScheduleInfo->Min_Level		= row_1.m_nMinLevel;
					pScheduleInfo->Max_Level		= row_1.m_nMaxLevel;
					pScheduleInfo->Min_PayingLevel	= row_1.m_nMinPayingLevel;
					pScheduleInfo->Max_PayingLevel	= row_1.m_nMaxPayingLevel;

					GLOBAL::SetBanServerFlag(std::wstring(row_1.m_szBanServer.GetString()), OUT pScheduleInfo->BanServer);
					GLOBAL::SetTargetServerFlag(std::wstring(row_1.m_szTargetServer.GetString()), OUT pScheduleInfo->TargetServer);

					pScheduleInfo->MaxRound = 0;
					CCustomEventInfoManager::Instance()->InsertSchedule(pScheduleInfo);
				}

				for (const auto& row_2 : repo_2)
				{
					auto pScheduleInfo = CCustomEventInfoManager::Instance()->GetScheduleInfo(row_2.m_nEventID);
					if (pScheduleInfo == nullptr)
						continue;

					pScheduleInfo->KeyItemKind     = row_2.m_nKeyItem_Kind;
					pScheduleInfo->KeyItemCount    = row_2.m_nKeyItem_Value;

					pScheduleInfo->BGImagePath     = std::wstring(row_2.m_szBGImage.GetString());
					pScheduleInfo->NumberImagePath = std::wstring(row_2.m_szNumberBGImage.GetString());
					pScheduleInfo->IconKey         = std::wstring(row_2.m_szIconKey.GetString());
					pScheduleInfo->IconSprite      = std::wstring(row_2.m_szIconSprite.GetString());
					pScheduleInfo->IconAtlas       = std::wstring(row_2.m_szIconAtlas.GetString());
					pScheduleInfo->HelpKey         = std::wstring(row_2.m_szHelpKey.GetString());
					pScheduleInfo->TitleKey        = std::wstring(row_2.m_szTitleKey.GetString());
				}

				for (const auto& row_3 : repo_3)
				{
					auto pRoundInfo = std::make_shared<RoundInfo>();
					if (pRoundInfo == nullptr)
						throw std::bad_alloc();

					pRoundInfo->EventID            = row_3.m_nEventID;
					pRoundInfo->Round              = row_3.m_nRound;
					pRoundInfo->RoundRewardGroupID = row_3.m_nRoundRewardGroupID;
					pRoundInfo->FixRewardGroupID   = row_3.m_nFixRewardGroupID;
					pRoundInfo->RoundImagePath     = std::wstring(row_3.m_szRoundImage.GetString());

					std::wstring NumString = L"";
					std::wstringstream WSS1(std::wstring(row_3.m_szRoundGoal.GetString()));
					auto pScheduleInfo = CCustomEventInfoManager::Instance()->GetScheduleInfo(pRoundInfo->EventID);
					while (std::getline(WSS1, NumString, L';'))
					{
						if (false == NumString.empty() && true == std::all_of(NumString.begin(), NumString.end(), iswdigit))
							pRoundInfo->RoundGoal.emplace_back(std::stoi(NumString));
					}

					if (pScheduleInfo != nullptr)
					{
						if(pScheduleInfo->EventType != eEventType::WARDICE)
							std::sort(pRoundInfo->RoundGoal.begin(), pRoundInfo->RoundGoal.end(), [](int a, int b) {return a < b; });

						pScheduleInfo->MaxRound = std::max(pScheduleInfo->MaxRound, pRoundInfo->Round);
					}

					CCustomEventInfoManager::Instance()->InsertRound(pRoundInfo);
				}

				for (const auto& row_4 : repo_4)
				{
					INT32 groupID                  = row_4.m_nRewardGroupID;
					INT32 rewardType               = row_4.m_nRewardType;
					GAME::eNATION nation           = static_cast<GAME::eNATION>(row_4.m_nNation);

					auto pRewardInfo = CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, nation, rewardType);
					if (pRewardInfo == nullptr)
					{
						pRewardInfo = std::make_shared<RewardInfo>();
						if (pRewardInfo == nullptr)
							throw std::bad_alloc();

						pRewardInfo->RewardGroupID = groupID;
						pRewardInfo->RewardType    = rewardType;
						pRewardInfo->Nation        = nation;

						pRewardInfo->RewardItems.emplace_back(BASE::REWARDITEM(row_4.m_nRewardKind, row_4.m_nRewardCount));

						CCustomEventInfoManager::Instance()->InsertReward(pRewardInfo);
					}
					else
					{
						bool isExist = false;
						for (int i = 0; i < pRewardInfo->RewardItems.size(); ++i)
						{
							if (pRewardInfo->RewardItems[i].Kind == row_4.m_nRewardKind)
							{
								pRewardInfo->RewardItems[i].Count += row_4.m_nRewardCount;
								isExist = true;
							}
						}

						if (false == isExist)
							pRewardInfo->RewardItems.emplace_back(BASE::REWARDITEM(row_4.m_nRewardKind, row_4.m_nRewardCount));
					}
				}

				for (const auto& row_5 : repo_5)
				{
					auto pProbabilityInfo = std::make_shared<ProbabilityInfo>();
					if (pProbabilityInfo == nullptr)
						throw std::bad_alloc();

					pProbabilityInfo->EventID     = row_5.m_nEventID;
					pProbabilityInfo->ProbKey_1   = row_5.m_nProbKey_[0];
					pProbabilityInfo->ProbKey_2   = row_5.m_nProbKey_[1];
					pProbabilityInfo->Probability = row_5.m_nProbability;

					CCustomEventInfoManager::Instance()->InsertProbability(pProbabilityInfo);
				}

				for (const auto& row_6 : repo_6)
				{
					auto pDeleteItemInfo = std::make_shared<DeleteItemInfo>();
					if (pDeleteItemInfo == nullptr)
						throw std::bad_alloc();
					
					pDeleteItemInfo->EventID		= row_6.m_nEventID;
					pDeleteItemInfo->ItemDeleteTime = row_6.m_nItemDeleteTime;
					pDeleteItemInfo->TitleKey		= std::wstring(row_6.m_szMailTitle.GetString());
					pDeleteItemInfo->ContentsKey	= std::wstring(row_6.m_szMailContents.GetString());

					std::wstring KeyItemTempString = L"";
					std::wstringstream WSS1(std::wstring(row_6.m_szKeyItem_Kind.GetString()));
					while (std::getline(WSS1, KeyItemTempString, L';'))
					{
						if (false == KeyItemTempString.empty() && true == std::all_of(KeyItemTempString.begin(), KeyItemTempString.end(), iswdigit))
							pDeleteItemInfo->KeyItemKinds.emplace_back(std::stoi(KeyItemTempString));
					}
					std::vector<std::wstring> itemInfos;
					std::vector<std::wstring> itemInfo;
					itemInfos.clear();

					UTIL::Split(itemInfos, row_6.m_szRewardItem.GetString(), L";");

					for (int i = 0; i < itemInfos.size(); ++i)
					{
						BASE::REWARDITEM item;
						item.Clear();
						itemInfo.clear();

						UTIL::ReplaceAll(itemInfos[i], L"(", L"");
						UTIL::ReplaceAll(itemInfos[i], L")", L"");

						UTIL::Split(itemInfo, itemInfos[i].c_str(), L":");

						if (itemInfo.size() > 1)
						{
							item.Kind = _ttoi(itemInfo[0].c_str());
							item.Count = _ttoi(itemInfo[1].c_str());
						}


						if(item.Kind != 0)
							pDeleteItemInfo->RewardItems.emplace_back(item);
					}

					CCustomEventInfoManager::Instance()->InsertDeleteItem(pDeleteItemInfo);
				}

				//포화속으로 이벤트로 새로 추가되는 일별 미션 정보
				for (const auto& row_7 : repo_7)
				{
					auto spQuestInfo = std::make_shared<QuestInfo>();
					if (spQuestInfo == nullptr)
						throw std::bad_alloc();

					spQuestInfo->GroupID = row_7.m_nGroupID;
					spQuestInfo->Day = row_7.m_nDay;
					spQuestInfo->QuestID = row_7.m_nQuestID;
					spQuestInfo->ConditionKind = row_7.m_nConditionKind;
					spQuestInfo->ConditionValue = row_7.m_nConditionValue;
					spQuestInfo->TargetValue = row_7.m_nTargetValue;

					spQuestInfo->PointHelpKey = std::wstring(row_7.m_szPointHelpKey.GetString());
					
					std::vector<std::wstring> itemInfos;
					std::vector<std::wstring> itemInfo;
					itemInfos.clear();

					UTIL::Split(itemInfos, row_7.m_szRewardItems.GetString(), L";");

					for (int i = 0; i < itemInfos.size(); ++i)
					{
						BASE::REWARDITEM item;
						item.Clear();
						itemInfo.clear();

						UTIL::ReplaceAll(itemInfos[i], L"(", L"");
						UTIL::ReplaceAll(itemInfos[i], L")", L"");

						UTIL::Split(itemInfo, itemInfos[i].c_str(), L":");

						if (itemInfo.size() <= 1)
							continue;

						item.Kind = _ttoi(itemInfo[0].c_str());
						item.Count = _ttoi(itemInfo[1].c_str());

						if (item.Kind != 0)
							spQuestInfo->RewardItems.emplace_back(item);
					}

					CCustomEventInfoManager::Instance()->InsertQuest(spQuestInfo);
				}

				for (const auto& row_8 : repo_8)
				{
					auto pValueInfo = std::make_shared<ValueInfo>();
					if (pValueInfo == nullptr)
						throw std::bad_alloc();

					pValueInfo->EventID = row_8.m_nEventID;
					pValueInfo->Difficulty = row_8.m_nDifficulty;
					pValueInfo->Value1 = row_8.m_nValue[0];
					pValueInfo->Value2 = row_8.m_nValue[1];
					pValueInfo->Value3 = row_8.m_nValue[2];
					pValueInfo->Value4 = row_8.m_nValue[3];
					pValueInfo->Value5 = row_8.m_nValue[4];
					pValueInfo->Value6 = row_8.m_nValue[5];
					pValueInfo->Value7 = row_8.m_nValue[6];
					pValueInfo->Value8 = row_8.m_nValue[7];
					pValueInfo->Value9 = row_8.m_nValue[8];
					pValueInfo->Value10 = row_8.m_nValue[9];

					CCustomEventInfoManager::Instance()->InsertValue(pValueInfo);
				}
				
				if (isServerStart)
				{
					NEW_FLATBUFFER(DB_CUSTOM_EVENT_INFO_LOAD_REQ, pPACKET);
					pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
					{ return PROTOCOL::FLATBUFFERS::CreateDB_CUSTOM_EVENT_INFO_LOAD_REQ(fbb, GLOBAL::GS_INFO.SVID); });
					SEND_DBA(pPACKET);
				}

				// 데이터가 리로드 되었으므로 map_CurSchedule을 새로 계산해야한다
				SetDirty();
				return  RECV_OK;
			});

			END_WDB_QUERY();

			return RECV_OK;
		});
}

void CCustomEventInfoManager::P_CUSTOM_EVENT_TEXT_LOAD(bool isServerStart)
{
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_WDB_QUERY_AUTO(P_CUSTOM_EVENT_TEXT_INFO_LOAD);
			RUN_WDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, repo_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				for (const auto& row_1 : repo_1)
				{
					std::wstring textKey = std::wstring(row_1.m_szTextKey.GetString());
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::KOR, textKey, std::wstring(row_1.m_szText_KOR.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::ENG, textKey, std::wstring(row_1.m_szText_ENG.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::ZHO, textKey, std::wstring(row_1.m_szText_ZHO.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::JPN, textKey, std::wstring(row_1.m_szText_JPN.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::NAN_, textKey, std::wstring(row_1.m_szText_NAN.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::GER, textKey, std::wstring(row_1.m_szText_GER.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::FRA, textKey, std::wstring(row_1.m_szText_FRA.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::IND, textKey, std::wstring(row_1.m_szText_IND.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::SPA, textKey, std::wstring(row_1.m_szText_SPA.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::POR, textKey, std::wstring(row_1.m_szText_POR.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::THA, textKey, std::wstring(row_1.m_szText_THA.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::ITA, textKey, std::wstring(row_1.m_szText_ITA.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::TUR, textKey, std::wstring(row_1.m_szText_TUR.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::ARA, textKey, std::wstring(row_1.m_szText_ARA.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::VIE, textKey, std::wstring(row_1.m_szText_VIE.GetString()));
					CCustomEventInfoManager::Instance()->InsertText(GAME::eLANGUAGE_CODE::RUS, textKey, std::wstring(row_1.m_szText_RUS.GetString()));
				}

				if (isServerStart)
				{
					NEW_FLATBUFFER(DB_CUSTOM_EVENT_TEXT_LOAD_REQ, pPACKET);
					pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
					{ return PROTOCOL::FLATBUFFERS::CreateDB_CUSTOM_EVENT_TEXT_LOAD_REQ(fbb, GLOBAL::GS_INFO.SVID); });
					SEND_DBA(pPACKET);
				}

				// 클라 전달
				if (false == isServerStart)
				{
					UpdateTextAllUser();
				}

				return  RECV_OK;
			});

			END_WDB_QUERY();

			return RECV_OK;
		});
}

void CCustomEventInfoManager::Send_DB_CUSTOM_EVENT_INFO_LOAD_REQ()
{
	P_CUSTOM_EVENT_INFO_LOAD(true);
}

INT8 CCustomEventInfoManager::Recv_DB_CUSTOM_EVENT_INFO_LOAD_ACK(const void* pData)
{
	LOGGER_DEBUG(CONST_EVENT_LOG, L"GAMEWORKERLOADER : Recv_DB_CUSTOM_EVENT_INFO_LOAD_ACK");
	return GameWorkerLoader::STATE::NEXT;
}

void CCustomEventInfoManager::Send_DB_CUSTOM_EVENT_TEXT_LOAD_REQ()
{
	P_CUSTOM_EVENT_TEXT_LOAD(true);
}

INT8 CCustomEventInfoManager::Recv_DB_CUSTOM_EVENT_TEXT_LOAD_ACK(const void* pData)
{
	LOGGER_DEBUG(CONST_EVENT_LOG, L"GAMEWORKERLOADER : Recv_DB_CUSTOM_EVENT_TEXT_LOAD_ACK");
	return GameWorkerLoader::STATE::NEXT;
}

void CCustomEventInfoManager::Send_DB_CUSTOM_USERDATA_LOAD_START()
{
	auto& repo = CUserManager::Instance()->GetRepository();
	for (auto iter = repo.begin(); iter != repo.end(); ++iter)
	{
		CUser::SharedPtr pUser = iter->second;
		if (IS_NULL(pUser) || false == pUser->IsPC())
			continue;

		m_UserIDs_ServerStartLoading.push(pUser->UID());
	}

	Send_DB_CUSTOM_USERDATA_LOAD_REQ();
}

void CCustomEventInfoManager::Send_DB_CUSTOM_USERDATA_LOAD_REQ()
{
	INT32 cnt = PAGE_LOAD_MAX;
	std::vector<INT64> userIDs;
	while (false == m_UserIDs_ServerStartLoading.empty() && cnt > 0)
	{
		INT64 userID = m_UserIDs_ServerStartLoading.top();
		m_UserIDs_ServerStartLoading.pop();
		userIDs.emplace_back(userID);
		cnt--;
	}

	CustomEventHelper::P_CUSTOM_EVENT_LOAD(userIDs, false, true);
}

INT8 CCustomEventInfoManager::Recv_DB_CUSTOM_USERDATA_LOAD_ACK(const void* pData)
{
	if (false == m_UserIDs_ServerStartLoading.empty())
	{
		Send_DB_CUSTOM_USERDATA_LOAD_REQ();
		return GameWorkerLoader::STATE::UPDATE;
	}

	LOGGER_DEBUG(CONST_EVENT_LOG, L"GAMEWORKERLOADER : Recv_DB_CUSTOM_USERDATA_LOAD_ACK");
	return GameWorkerLoader::STATE::NEXT;
}

void CCustomEventInfoManager::Recv_GS_CUSTOM_EVENT_TEXT_GET_REQ(CUser* const pUser, const void* pData)
{
	if (nullptr == pUser)
		return;

	auto pReq = PROTOCOL::FLATBUFFERS::GetGS_CUSTOM_EVENT_TEXT_GET_REQ(pData);
	if (nullptr == pReq)
		return;

	auto languageType = pReq->LanguageCode();
	auto find_iter = repo_Text.find((GAME::eLANGUAGE_CODE)languageType);
	if (find_iter != repo_Text.end())
	{
		NEW_FLATBUFFER(GS_CUSTOM_EVENT_TEXT_GET_ACK, pPACKET);
		pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
		{
			FlatBufferContainer<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_TEXT_INFO> flatVecTextInfos;
			for (const auto& [textKey, textValue] : (*find_iter).second->LanguageMap)
			{
				auto flat = PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_TEXT_INFO(fbb, CREATE_FLATBUFFER_STRING(textKey), CREATE_FLATBUFFER_STRING(textValue));
				flatVecTextInfos.push_back(flat);
			}

			return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_TEXT_GET_ACK(fbb,
				languageType, fbb.CreateVector(flatVecTextInfos));
		});
		SEND_ACTIVE_USER(pUser, pPACKET);
	}
}

void CCustomEventInfoManager::UpdateTextAllUser()
{
	for (auto itr = CSessionInfoManager::Instance()->GetRepository().begin(); itr != CSessionInfoManager::Instance()->GetRepository().end(); ++itr)
	{
		CUser* const pUser = CSessionInfoManager::GetUser(itr);
		if (nullptr == pUser)
			continue;

		if (false == pUser->IsActive())
			continue;

		auto languageType = pUser->GetLanguageType();
		auto find_iter = repo_Text.find((GAME::eLANGUAGE_CODE)languageType);
		if (find_iter != repo_Text.end())
		{
			NEW_FLATBUFFER(GS_CUSTOM_EVENT_TEXT_UPDATE_NFY, pPACKET);
			pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				FlatBufferContainer<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_TEXT_INFO> flatVecTextInfos;
				for (const auto& [textKey, textValue] : (*find_iter).second->LanguageMap)
				{
					auto flat = PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_TEXT_INFO(fbb, CREATE_FLATBUFFER_STRING(textKey), CREATE_FLATBUFFER_STRING(textValue));
					flatVecTextInfos.push_back(flat);
				}

				return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_TEXT_UPDATE_NFY(fbb,
					languageType, fbb.CreateVector(flatVecTextInfos));
			});

			SEND_ACTIVE_USER(pUser, pPACKET);
		}
	}
}

}// namespace CustomEvent


