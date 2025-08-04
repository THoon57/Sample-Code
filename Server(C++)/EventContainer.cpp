#include "GameAfx.h"

#include "_DatabaseThreadManager/DBQUERY/GameDB.h"
#include "_DatabaseThreadManager/DBAGlobal.h"
#include "_DatabaseThreadManager/DBQUERY/GDB/P_CUSTOM_EVENT_LOAD.h"
#include "_DatabaseThreadManager/DBQUERY/GDB/P_CUSTOM_EVENT_SCHEDULE_SET.h"
#include "_DatabaseThreadManager/DBQUERY/GDB/P_CUSTOM_EVENT_UPDATE.h"
#include "_DatabaseThreadManager/DBQUERY/GDB/P_CUSTOM_EVENT_COMPLETE.h"

bool CCustumEventBase::SetReward(const INT32 round, const INT32 rewardType, const INT32 rewardValue)
{
	auto tupleKey = std::make_tuple(round, rewardType);
	if (m_RewardMap.end() != m_RewardMap.find(tupleKey))
	{
		CCustumEventReward::SharedPtr spReward = m_RewardMap.at(tupleKey);
		if (spReward->m_RewardValue == rewardValue)
			return false;

		spReward->m_RewardValue = rewardValue;
	}
	else if (rewardValue == 0)
	{
		return false;
	}
	else
	{
		CCustumEventReward::SharedPtr spReward = std::make_shared<CCustumEventReward>();
		if (spReward == nullptr)
			throw std::bad_alloc();

		spReward->m_Round = round;
		spReward->m_RewardType = rewardType;
		spReward->m_RewardValue = rewardValue;

		if (false == m_RewardMap.emplace(tupleKey, spReward).second)
			return false;
	}

	return true;
}

bool CCustumEventBase::IsRewarded(const INT32 round, const INT32 rewardType)
{
	auto tupleKey = std::make_tuple(round, rewardType);
	if (m_RewardMap.end() != m_RewardMap.find(tupleKey))
	{
		CCustumEventReward::SharedPtr spReward = m_RewardMap.at(tupleKey);
		if (spReward != nullptr)
			return (spReward->m_RewardValue > 0);
	}

	return false;
}

void CCustumEventBase::GetReward(const INT32 round, OUT std::vector<INT32>& out_RewardTypes)
{
	out_RewardTypes.clear();

	for (const auto& pair : m_RewardMap)
	{
		const auto& spReward = pair.second;
		if (spReward == nullptr || spReward->m_Round != round)
			continue;

		if (spReward->m_RewardValue <= 0)
			continue;

		out_RewardTypes.emplace_back(spReward->m_RewardType);
	}
}

INT32 CCustumEventBase::GetRewardValue(const INT32 round, const INT32 rewardType)
{
	auto iter = m_RewardMap.find(std::pair(round, rewardType));
	if (iter != m_RewardMap.end())
	{
		return iter->second->m_RewardValue;
	}

	return 0;
}

bool CBingoEvent::SetProgress(const INT32 bingoNum)
{
	INT32 rowIdx = static_cast<INT32>(bingoNum / CustomEvent::BINGO_ROW_COUNT);
	INT32 colIdx = static_cast<INT32>(bingoNum % CustomEvent::BINGO_ROW_COUNT);
	
	if (false == m_BingoBoard[rowIdx][colIdx])
	{
		m_BingoBoard[rowIdx][colIdx] = true;
		return true;
	}
	else
	{
		return false;
	}
}

void CBingoEvent::ClearProgress()
{
	for (INT32 rowIdx = 0; rowIdx < CustomEvent::BINGO_ROW_COUNT; rowIdx++)
	{
		for (INT32 colIdx = 0; colIdx < CustomEvent::BINGO_ROW_COUNT; colIdx++)
		{
			m_BingoBoard[rowIdx][colIdx] = false;
		}
	}
}

bool CRouletteEvent::SetProgress(const INT32 value)
{
	m_Score = value;
	return true;
}

void CRouletteEvent::ClearProgress()
{
	m_Score = 0;
}

CCustomEventContainer::EventDatum::SharedPtr CCustomEventContainer::SeekDatum(INT64 nKey)
{
	auto iterF = repo_Event.find(nKey);
	if (iterF == repo_Event.end())
		return nullptr;

	return (*iterF).second;
}

CCustumEventBase::SharedPtr CCustomEventContainer::Insert(CCustumEventBase::SharedPtr newData)
{
	if (!newData)
		return nullptr;

	if (SeekDatum(newData->m_EventID) != nullptr)
		return nullptr;

	auto spDatum = std::make_shared<EventDatum>();
	if (!spDatum)
		throw std::bad_alloc();

	auto [iter_1, result_1] = repo_Event.emplace(newData->m_EventID, spDatum);
	if (!result_1)
		return nullptr;

	spDatum->pEvent = newData;
	spDatum->iter_Event = iter_1;
	return newData;
}

void CCustomEventContainer::Delete(INT64 nKey)
{
	auto spDatum = SeekDatum(nKey);
	if (!spDatum)
		return;

	repo_Event.erase(spDatum->iter_Event);
}

bool CCustomEventContainer::UpdateQuestCacheMap()
{
	repo_Quest.clear();

	std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecNeedUpdate;

	for (const auto& pair1 : repo_Event)
	{
		auto& pEvent = (pair1).second->pEvent;
		if (pEvent == nullptr || pEvent->m_State != CustomEvent::eEventState::PROCESSING)
			continue;

		for (const auto& pair2 : pEvent->m_QuestMap)
		{
			auto& pQuest = pair2.second;
			if (pQuest == nullptr)
				continue;

			if (pQuest->m_MissionState != CustomEvent::eCustomEventMissionState::PROCESSIONG)
				continue;

			CustomEvent::QuestInfo::SharedPtr spQeustInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfo(pEvent->m_EventID, pQuest->m_QuestID);
			if (spQeustInfo == nullptr)
				continue;

			repo_Quest.emplace(spQeustInfo->ConditionKind, pQuest);

			const INT64 prevValue = pQuest->m_CurValue;
			const CustomEvent::eCustomEventMissionState prevState = pQuest->m_MissionState;

			INT64 afterValue = prevValue;
			CustomEvent::eCustomEventMissionState afterState = prevState;

			// 스칼라값
			const INT64 scalarValue = GLOBAL::GetMissionScalar(m_UserID, spQeustInfo->ConditionKind, spQeustInfo->ConditionValue, pQuest->m_CurValue);

			if (afterValue < scalarValue)
				afterValue = scalarValue;
			if (spQeustInfo->TargetValue <= afterValue)
				afterState = CustomEvent::eCustomEventMissionState::COMPLETE;

			if (prevValue != afterValue || prevState != afterState)
			{
				vecNeedUpdate.emplace_back(std::make_tuple(pEvent->m_EventID, pQuest->m_QuestID, afterValue, afterState));
			}
		}
	}

	if (vecNeedUpdate.empty() == false)
	{
		if (CUser* pUser = CUserManager::Instance()->FindByUID(m_UserID))
			CustomEventHelper::P_CUSTOMEEVENT_QUEST_SET_TV(pUser, vecNeedUpdate);
	}


	return true;
}

set<INT64> CCustomEventContainer::GetEventID_By_ScheduleType(CustomEvent::eScheduleType eScheduleType)
{
	set<INT64> resultID;

	auto& curEventMap = GetEventRepository();

	for (const auto& [eventID, eventInfo] : curEventMap)
	{
		if (eventInfo == nullptr)
			continue;

		const auto event_NDT_Info = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
		if (event_NDT_Info == nullptr)
			continue;

		if (event_NDT_Info->ScheduleType != eScheduleType)
			continue;

		resultID.emplace(eventID);
	}

	return resultID;
}

void CustomEventHelper::OnUserCreate(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	CustomEventHelper::CheckRefreshTime(pUser.get(), true);
}

void CustomEventHelper::OnUserLogin(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	if (pUser->GetNation() != GAME::eNATION::eNATION_NONE)
		CustomEventHelper::CheckRefreshTime(pUser.get(), true);

	// Send Text
	auto languageType = pUser->GetLanguageType();
	auto textInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetTextInfo((GAME::eLANGUAGE_CODE)languageType);
	if (textInfo == nullptr)
		return;

	NEW_FLATBUFFER(GS_CUSTOM_EVENT_TEXT_UPDATE_NFY, pPACKET);
	pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{
		FlatBufferContainer<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_TEXT_INFO> flatVecTextInfos;
		for (auto& pair : textInfo->LanguageMap)
		{
			auto flat = PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_TEXT_INFO(fbb, CREATE_FLATBUFFER_STRING(pair.first), CREATE_FLATBUFFER_STRING(pair.second));
			flatVecTextInfos.push_back(flat);
		}

		return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_TEXT_UPDATE_NFY(fbb,
			languageType, fbb.CreateVector(flatVecTextInfos));
	});

	SEND_ACTIVE_USER(pUser, pPACKET);
}

void CustomEventHelper::OnUserLevelUp(CUser* const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	ASE_INSTANCE(pUser, CCustomEventContainer)->SetDirty();
	CustomEventHelper::CheckRefreshTime(pUser, true);
}

void CustomEventHelper::OnUserComeback(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	CustomEventHelper::P_CUSTOM_EVENT_LOAD({ pUser->UID() }, true, false);
}

bool CustomEventHelper::OnQuestEvent(INT64 userID, INT32 conditionKind, INT64 conditionValue, INT64 value, CQuestManager::SETTYPE settype)
{
	if (GLOBAL::IsBattleRoyalServer() || CServerInvasionManager::Instance()->IsInvasionUser(userID))
		return false;

	CUser* pUser = CUserManager::Instance()->FindByUID(userID);
	if (pUser == nullptr || pUser->IsPC() == false)
		return false;

	CustomEventHelper::CheckRefreshTime(pUser);

	std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecNeedUpdate;

	auto& questMap = ASE_INSTANCE(pUser, CCustomEventContainer)->GetQuestRepository();

	for (const auto& [key, quest] : GAME::for_range(questMap.equal_range(conditionKind)))
	{
		CCustomEventQuest::SharedPtr pQuest = quest.lock();
		if (pQuest == nullptr)
			continue;

		if (pQuest->m_MissionState != CustomEvent::eCustomEventMissionState::PROCESSIONG)
			continue;

		CustomEvent::QuestInfo::SharedPtr pQuestInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfo(pQuest->m_EventID, pQuest->m_QuestID);
		if (pQuestInfo == nullptr)
			continue;

		if (pQuestInfo->ConditionKind != conditionKind)
			continue;

		const CCustumEventBase::SharedPtr pEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(pQuest->m_EventID);
		if (pEvent == nullptr)
			continue;

		if (pEvent->m_State != CustomEvent::eEventState::PROCESSING)
			continue;

		auto scheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(pEvent->m_EventID);
		if (scheduleInfo == nullptr)
			continue;

		INT64 prevValue = pQuest->m_CurValue;
		INT64 afterValue = pQuest->m_CurValue;

		if (BASE::QUEST_CONDITION_INFO::CanUpdateValue(pQuestInfo->ConditionKind, pQuestInfo->ConditionValue, conditionValue))
		{
			if (settype == CQuestManager::SET)
				afterValue = std::max(afterValue, value);
			else if (settype == CQuestManager::INCREASE)
				afterValue += value;
		}

		if (afterValue > prevValue)
		{
			CustomEvent::eCustomEventMissionState afterState = CustomEvent::eCustomEventMissionState::PROCESSIONG;

			if (afterValue >= pQuestInfo->TargetValue)
				afterState = CustomEvent::eCustomEventMissionState::COMPLETE;

			vecNeedUpdate.emplace_back(std::make_tuple(pEvent->m_EventID, pQuest->m_QuestID, afterValue, afterState));
		}
	}
	if (vecNeedUpdate.empty() == false)
	{
		CustomEventHelper::P_CUSTOMEEVENT_QUEST_SET_TV(pUser, vecNeedUpdate, true);
		return true;
	}

	return false;
}

bool CustomEventHelper::CheckRefreshTime(CUser* const pUser, bool sendClient)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return false;

	if (GLOBAL::IsBattleRoyalServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	INT64 refreshTime = ASE_INSTANCE(pUser, CCustomEventContainer)->GetRefreshTime();
	bool needNFY = false;

	if (false == CheckSameHour_UTC(refreshTime, curTime))
	{
		needNFY |= CustomEventHelper::CheckEventExpired(pUser, curTime);
		needNFY |= CustomEventHelper::CheckEventOpen(pUser, curTime);
		needNFY |= CustomEventHelper::CheckDeleteItem(pUser, curTime);

		ASE_INSTANCE(pUser, CCustomEventContainer)->UpdateQuestCacheMap();
		ASE_INSTANCE(pUser, CCustomEventContainer)->SetRefreshTime(curTime);
	}

	if (sendClient || needNFY)
		Send_GS_CUSTOM_EVENT_INFO_NFY(pUser);
	return needNFY;
}

bool CustomEventHelper::CheckDeleteItem(CUser* const pUser, const INT64 curTime)
{
	bool result = false;
	if (pUser == nullptr)
		return false;

	auto& curEventMap = ASE_INSTANCE(pUser, CCustomEventContainer)->GetEventRepository();

	for (const auto& pair : curEventMap)
	{
		auto& pEvent = pair.second->pEvent;
		if (pEvent == nullptr)
			continue;

		if (pEvent->m_State == CustomEvent::eEventState::ITEMDELETE)
			continue;
		
		auto deleteItemInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetDeleteItemInfo(pEvent->m_EventID);
		if (deleteItemInfo == nullptr)
			continue;
	
		auto scheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(pEvent->m_EventID);
		if (scheduleInfo == nullptr)
			continue;

		// 진행중이라면..
		if (curTime >= pEvent->m_StartTime || pEvent->m_EndTime > curTime)
		{
			if (pEvent->m_State == CustomEvent::eEventState::PROCESSING)
				continue;
		}

		INT64 itemDeleteTime = 0;

		if (scheduleInfo->ScheduleType == CustomEvent::eScheduleType::UTC_DATE)
			itemDeleteTime = deleteItemInfo->ItemDeleteTime;
		else if (scheduleInfo->ScheduleType == CustomEvent::eScheduleType::SERVER_AGE)
			itemDeleteTime = EventHelper::GetAdjustTime(GetServerCreateTime()) + (static_cast<INT64>(SECOND_PER_DAY) * deleteItemInfo->ItemDeleteTime - 1);
		else if (scheduleInfo->ScheduleType == CustomEvent::eScheduleType::USER_AGE)
			itemDeleteTime = pUser->GetCreateTime() + (static_cast<INT64>(SECOND_PER_DAY) * deleteItemInfo->ItemDeleteTime - 1);

		//아직 안 지워도 된다.
		if (curTime < itemDeleteTime)
			continue;

		P_CUSTOM_EVENT_ITEMDELETE(pUser, deleteItemInfo->EventID);

		result = true;
	}

	return result;
}

bool CustomEventHelper::CheckEventExpired(CUser* const pUser, const INT64 curTime)
{
	if (pUser == nullptr)
		return false;

	auto& eventInfoMap = CustomEvent::CCustomEventInfoManager::Instance()->GetCurEventMap();
	auto& curEventMap = ASE_INSTANCE(pUser, CCustomEventContainer)->GetEventRepository();

	bool ret = false;

	for (const auto& pair1 : curEventMap)
	{
		auto& pEvent = (pair1).second->pEvent;
		if (pEvent == nullptr)
			continue;

		// 미리 계산해둔 시간에서 벗어났다면 EXPIRED
		if (curTime < pEvent->m_StartTime || pEvent->m_EndTime <= curTime)
		{
			if (pEvent->m_State == CustomEvent::eEventState::PROCESSING )
			{
				ret = true;
				P_CUSTOM_EVENT_COMPLETE(pUser, pEvent->m_EventID);
			}
			continue;
		}

		// NDT 정보에서 없어졌다면 EXPIRED
		if (eventInfoMap.end() == eventInfoMap.find(pEvent->m_EventID))
		{
			if (pEvent->m_State == CustomEvent::eEventState::PROCESSING)
			{
				ret = true;
				P_CUSTOM_EVENT_COMPLETE(pUser, pEvent->m_EventID);
			}
			continue;
		}

		if (pEvent->m_State != CustomEvent::eEventState::PROCESSING)
			ret = true;
		pEvent->m_State = CustomEvent::eEventState::PROCESSING;
	}

	return ret;
}

bool CustomEventHelper::CheckEventOpen(CUser* const pUser, const INT64 curTime)
{
	if (pUser == nullptr)
		return false;
	
	auto& eventInfoMap = CustomEvent::CCustomEventInfoManager::Instance()->GetCurEventMap();
	auto& curEventMap = ASE_INSTANCE(pUser, CCustomEventContainer)->GetEventRepository();

	INT64 serverCreateTime = EventHelper::GetAdjustTime(GetServerCreateTime());
	INT32 userCreateTime = EventHelper::GetAdjustTime(pUser->GetCreateTime());
	INT32 commandLv = pUser->Territory().GetBuildLevelFromKind(GAME::KIND_BUILD_COMMAND);
	INT32 payingLv = pUser->Get_PayingLv();

	std::vector<std::tuple<INT64, INT64, INT64>> registEvent;

	for (const auto& pair2 : eventInfoMap)
	{
		auto pInfo = (pair2).second.lock();
		if (pInfo == nullptr)
			continue;

		// 이미 등록되어 있는 이벤트라면 통과
		if (curEventMap.end() != curEventMap.find(pInfo->EventID))
			continue;

		if (pInfo->EventType <= CustomEvent::eEventType::NONE || CustomEvent::eEventType::END <= pInfo->EventType)
			continue;

		// 사령부 레벨 필터
		if (pInfo->Min_Level > 0 || pInfo->Max_Level > 0)
		{
			if (commandLv < pInfo->Min_Level || pInfo->Max_Level < commandLv)
				continue;
		}

		// 페잉 레벨 필터
		if (pInfo->Min_PayingLevel > 0 || pInfo->Max_PayingLevel > 0)
		{
			if (payingLv < pInfo->Min_PayingLevel || pInfo->Max_PayingLevel < payingLv)
				continue;
		}

		INT64 startTime = 0;
		INT64 endTime = 0;
		INT64 duration = std::max(pInfo->EventDuration, pInfo->RewardDuration);

		if (pInfo->ScheduleType == CustomEvent::eScheduleType::UTC_DATE)
		{
			startTime = pInfo->ScheduleValue;
			endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);
		}
		else if (pInfo->ScheduleType == CustomEvent::eScheduleType::SERVER_AGE)
		{
			startTime = serverCreateTime + (static_cast<INT64>(SECOND_PER_DAY) * (pInfo->ScheduleValue - 1));
			endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);
		}
		else if (pInfo->ScheduleType == CustomEvent::eScheduleType::USER_AGE)
		{
			startTime = userCreateTime + (static_cast<INT64>(SECOND_PER_DAY) * (pInfo->ScheduleValue - 1));
			endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * duration);
		}

		if (startTime < 0 || endTime < 0 || curTime < startTime || endTime < curTime)
			continue;

		// 유저 나이 필터
		if (pInfo->Min_UserAge > 0 || pInfo->Max_UserAge > 0)
		{
			INT32 startUserAge = EventHelper::GetAdjustDay(userCreateTime, startTime);
			INT32 endUserAge = EventHelper::GetAdjustDay(userCreateTime, endTime);
			if (startUserAge < pInfo->Min_UserAge || pInfo->Max_UserAge < endUserAge)
				continue;
		}

		// 서버 나이 필터
		if (pInfo->Min_ServerAge > 0 || pInfo->Max_ServerAge > 0)
		{
			INT32 startServerAge = EventHelper::GetAdjustDay(serverCreateTime, startTime);
			INT32 endServerAge = EventHelper::GetAdjustDay(serverCreateTime, endTime);
			if (startServerAge < pInfo->Min_ServerAge || pInfo->Max_ServerAge < endServerAge)
				continue;
		}

		registEvent.emplace_back(std::make_tuple(pInfo->EventID, startTime, endTime));
	}

	if (false == registEvent.empty())
	{
		P_CUSTOM_EVENT_SCHEDULE_SET(pUser, registEvent);
		return true;
	}

	return false;
}


bool CustomEventHelper::P_CUSTOM_EVENT_LOAD(const std::vector<INT64> userIDs, const bool sendClient, const bool serverStart)
{
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_LOAD);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->TVP_1.clear();
			for (INT64 userid : userIDs)
				pQuery->TVP_1.m_nUserID.emplace_back(userid);
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [sendClient, serverStart,
				repo_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2(), repo_3 = pQuery->GetSET_3(), repo_4 = pQuery->GetSET_4()]()->RECV_RESULT
			{
				for (const auto& row_1 : repo_1)
				{
					CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(row_1.m_nEventID);
					if (pEventInfo == nullptr)
						continue;

					CUser* pUser = CUserManager::Instance()->FindByUID(row_1.m_nUserID);
					if (pUser == nullptr)
						continue;

					if (GLOBAL::IsBattleRoyalServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
						continue;

					CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(row_1.m_nEventID);
					if (spEvent != nullptr)
					{
						spEvent->m_EventID = row_1.m_nEventID;
						auto dbstampvalue = row_1.m_tmStartTime;
						spEvent->m_StartTime = std::max(0LL, TLDB::TIME_DB2UTC(dbstampvalue));
						dbstampvalue = row_1.m_tmEndTime;
						spEvent->m_EndTime = std::max(0LL, TLDB::TIME_DB2UTC(dbstampvalue));
						spEvent->m_State = static_cast<CustomEvent::eEventState>(row_1.m_nState);

						spEvent->m_Round = 1;	// 플레이를 진행하지 않았다면 repo_2에 정보가 없으므로 일단 1로 초기화
						spEvent->m_RewardMap.clear();
						spEvent->m_QuestMap.clear();
					}
					else
					{
						switch (pEventInfo->EventType)
						{
						case CustomEvent::eEventType::BINGO:		spEvent = std::make_shared<CBingoEvent>();    break;
						case CustomEvent::eEventType::ROULETTE:		spEvent = std::make_shared<CRouletteEvent>(); break;
						case CustomEvent::eEventType::TIMINGGAME:	spEvent = std::make_shared<CTimingGameEvent>(); break;
						case CustomEvent::eEventType::WARDICE:		spEvent = std::make_shared<CWarDiceEvent>(); break;
						default:									spEvent = nullptr;                            break;
						}

						if (spEvent == nullptr)
						{
							LOGGER_ERROR(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_LOAD - Invalid EventType:{}", static_cast<UINT8>(pEventInfo->EventType));
							continue;
						}

						spEvent->m_EventID = row_1.m_nEventID;
						auto dbstampvalue = row_1.m_tmStartTime;
						spEvent->m_StartTime = std::max(0LL, TLDB::TIME_DB2UTC(dbstampvalue));
						dbstampvalue = row_1.m_tmEndTime;
						spEvent->m_EndTime = std::max(0LL, TLDB::TIME_DB2UTC(dbstampvalue));
						spEvent->m_State = static_cast<CustomEvent::eEventState>(row_1.m_nState);

						spEvent->m_Round = 1;	// 플레이를 진행하지 않았다면 repo_2에 정보가 없으므로 일단 1로 초기화
						spEvent->m_RewardMap.clear();
						spEvent->m_QuestMap.clear();

						ASE_INSTANCE(pUser, CCustomEventContainer)->Insert(spEvent);
					}
					CustomEventHelper::RegistQuest(spEvent);
					ASE_INSTANCE(pUser, CCustomEventContainer)->SetUserID(row_1.m_nUserID);
				}

				for (const auto& row_2 : repo_2)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_2.m_nUserID);
					if (pUser == nullptr)
						continue;

					CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(row_2.m_nEventID);
					if (spEvent == nullptr)
						continue;

					spEvent->m_Round = row_2.m_nRound;
					spEvent->m_RewardKind = row_2.m_nRewardKind;

					if (auto spBingoEvent = std::dynamic_pointer_cast<CBingoEvent>(spEvent))
						CustomEventHelper::ConvertBingoBoardFromBitFlag(row_2.m_nProgress, OUT spBingoEvent->m_BingoBoard);
					else if (auto spRouletteEvent = std::dynamic_pointer_cast<CRouletteEvent>(spEvent))
						spRouletteEvent->m_Score = row_2.m_nProgress;
					else if (auto spTimingGameEvent = std::dynamic_pointer_cast<CTimingGameEvent>(spEvent))
						spTimingGameEvent->m_Score = row_2.m_nProgress;
					else if (auto spWarDiceEvent = std::dynamic_pointer_cast<CWarDiceEvent>(spEvent))
						spWarDiceEvent->m_DiceBlock = row_2.m_nProgress;
				}

				for (const auto& row_3 : repo_3)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_3.m_nUserID);
					if (pUser == nullptr)
						continue;

					CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(row_3.m_nEventID);
					if (spEvent == nullptr)
						continue;

					auto spReward = std::make_shared<CCustumEventReward>();
					if (spReward == nullptr)
						throw std::bad_alloc();

					spReward->m_Round = spEvent->m_Round;
					spReward->m_RewardType = row_3.m_nRewardType;
					spReward->m_RewardValue = row_3.m_nRewardValue;

					spEvent->m_RewardMap.emplace(std::make_tuple(spReward->m_Round, spReward->m_RewardType), spReward);
				}

				// T_CustomEvent_Quest
				for(const auto& row_4 : repo_4)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_4.m_nUserID);
					if (pUser == nullptr)
						continue;

					CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(row_4.m_nEventID);
					if (spEvent == nullptr)
						continue;

					auto questIter = spEvent->m_QuestMap.find(row_4.m_nQuestID);
					if (spEvent->m_QuestMap.end() != questIter)
					{
						questIter->second->m_EventID = row_4.m_nEventID;
						questIter->second->m_QuestID = row_4.m_nQuestID;
						questIter->second->m_CurValue = row_4.m_nCurValue;
						questIter->second->m_MissionState = static_cast<CustomEvent::eCustomEventMissionState>(row_4.m_nState);
					}
					else
					{
						auto spQuest = std::make_shared<CCustomEventQuest>();
						if (spQuest == nullptr)
							throw std::bad_alloc();

						spQuest->m_EventID = row_4.m_nEventID;
						spQuest->m_QuestID = row_4.m_nQuestID;
						spQuest->m_CurValue = row_4.m_nCurValue;
						spQuest->m_MissionState = static_cast<CustomEvent::eCustomEventMissionState>(row_4.m_nState);

						spEvent->m_QuestMap.emplace(spQuest->m_QuestID, spQuest);
					}
				}

				if (sendClient)
				{
					for (const auto& row_1 : repo_1)
					{
						CUser* pUser = CUserManager::Instance()->FindByUID(row_1.m_nUserID);
						if (pUser == nullptr)
							continue;

						ASE_INSTANCE(pUser, CCustomEventContainer)->SetDirty();
						CustomEventHelper::CheckRefreshTime(pUser, true);
					}
				}

				if (serverStart)
				{
					NEW_FLATBUFFER(DB_CUSTOM_EVENT_DATA_LOAD_REQ, pPACKET);
					pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
					{ return PROTOCOL::FLATBUFFERS::CreateDB_CUSTOM_EVENT_DATA_LOAD_REQ(fbb, GLOBAL::GS_INFO.SVID); });
					SEND_DBA(pPACKET);
				}
				return  RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}

bool CustomEventHelper::P_CUSTOM_EVENT_SCHEDULE_SET(CUser* const pUser, const std::vector<std::tuple<INT64, INT64, INT64>> vecInfos)
{
	if (pUser == nullptr)
		return false;

	INT64 userID = pUser->UID();

	for (const auto& tuple : vecInfos)
	{
		INT64 eventID = std::get<0>(tuple);
		INT64 startTime = std::get<1>(tuple);
		INT64 endTime = std::get<2>(tuple);

		CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
		if (pEventInfo == nullptr)
			continue;

		CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
		if (spEvent != nullptr)
		{
			spEvent->m_EventID = eventID;
			spEvent->m_StartTime = startTime;
			spEvent->m_EndTime = endTime;
			spEvent->m_Round = 1;
			spEvent->m_RewardKind = 0;
			spEvent->m_RewardMap.clear();
			spEvent->m_QuestMap.clear();

			spEvent->m_State = CustomEvent::eEventState::PROCESSING;
		}
		else
		{
			switch (pEventInfo->EventType)
			{
			case CustomEvent::eEventType::BINGO:		spEvent = std::make_shared<CBingoEvent>();			break;
			case CustomEvent::eEventType::ROULETTE:		spEvent = std::make_shared<CRouletteEvent>();		break;
			case CustomEvent::eEventType::TIMINGGAME:	spEvent = std::make_shared<CTimingGameEvent>();		break;
			case CustomEvent::eEventType::WARDICE:		spEvent = std::make_shared<CWarDiceEvent>();		break;
			default:									spEvent = nullptr;									break;
			}

			if (spEvent == nullptr)
			{
				LOGGER_ERROR(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_SCHEDULE_SET - Invalid EventType:{}", static_cast<UINT8>(pEventInfo->EventType));
				continue;
			}

			spEvent->m_EventID = eventID;
			spEvent->m_StartTime = startTime;
			spEvent->m_EndTime = endTime;
			spEvent->m_Round = 1;
			spEvent->m_RewardKind = 0;
			spEvent->m_RewardMap.clear();

			spEvent->m_State = CustomEvent::eEventState::PROCESSING;
			ASE_INSTANCE(pUser, CCustomEventContainer)->Insert(spEvent);
		}

		CustomEventHelper::RegistQuest(spEvent);
	}
	//INT32 nServerID = pUser->GetServerID();
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_SCHEDULE_SET);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;

			pQuery->TVP_1.clear();
			for (const auto& tuple : vecInfos)
			{
				pQuery->TVP_1.m_nEventID.push_back(std::get<0>(tuple));
				pQuery->TVP_1.m_nStartTime.push_back(TLDB::TIME_UTC2DB(std::get<1>(tuple)));
				pQuery->TVP_1.m_nEndTime.push_back(TLDB::TIME_UTC2DB(std::get<2>(tuple)));
				pQuery->TVP_1.m_nState.push_back(static_cast<UINT8>(CustomEvent::eEventState::PROCESSING));
			}
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_SCHEDULE_SET_TV. Fail Result:{}", SET_1[0].m_nResult);

						for (const auto& tuple : vecInfos)
							ASE_INSTANCE(pResultUser, CCustomEventContainer)->Delete(std::get<0>(tuple));
						ASE_INSTANCE(pResultUser, CCustomEventContainer)->SetDirty();
						return RECV_OK;
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_SCHEDULE_SET_TV. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}
bool CustomEventHelper::RegistQuest(CCustumEventBase::SharedPtr newData)
{
	if (newData == nullptr)
		return false;

	if (newData->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	newData->m_QuestMap.clear();

	auto pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(newData->m_EventID);
	if (pEventInfo == nullptr)
		return false;

	auto questMap = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfos(pEventInfo->QuestGroup);
	if (questMap == nullptr)
		return false;

	for (const auto& [_, pQuestInfo] : *questMap)
	{
		auto spQuest = std::make_shared<CCustomEventQuest>();
		if (spQuest == nullptr)
			throw std::bad_alloc();

		spQuest->m_EventID = newData->m_EventID;
		spQuest->m_QuestID = pQuestInfo->QuestID;
		spQuest->m_CurValue = 0;
		spQuest->m_MissionState = CustomEvent::eCustomEventMissionState::PROCESSIONG;

		newData->m_QuestMap.emplace(spQuest->m_QuestID, spQuest);
	}

	return true;
}

bool CustomEventHelper::P_CUSTOM_EVENT_UPDATE_SelectRewardKind(CUser* const pUser, const INT64 eventID, const INT32 rewardKind)
{
	// Select Round RewardKind

	if (pUser == nullptr)
		return false;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < spEvent->m_StartTime || spEvent->m_EndTime < curTime)
		return false;

	if (spEvent->m_RewardKind > 0 || spEvent->m_RewardKind == rewardKind)
		return false;

	if (false == EventHelper::CheckValidItem(rewardKind, curTime))
		return false;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return false;

	INT32 groupID = pRoundInfo->RoundRewardGroupID;

	CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), 0);
	if (pRewardInfo == nullptr)
		pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, 0);
	if (pRewardInfo == nullptr)
		return false;

	INT32 rewardItemCount = -1;
	for (const auto& rewardItem : pRewardInfo->RewardItems)
	{
		if (rewardItem.Kind == rewardKind)
		{
			rewardItemCount = rewardItem.Count;
			break;
		}
	}

	if (rewardItemCount < 0)
		return false;

	// 기존 값 backup
	INT32 progress = 0;
	if (CBingoEvent::SharedPtr spBingoEvent = std::dynamic_pointer_cast<CBingoEvent>(spEvent))
		ConvertBingoBoardToBitFlag(spBingoEvent->m_BingoBoard, OUT progress);
	else if (CRouletteEvent::SharedPtr spRouletteEvent = std::dynamic_pointer_cast<CRouletteEvent>(spEvent))
		progress = spRouletteEvent->m_Score;
	else if (CTimingGameEvent::SharedPtr spTimingGameEvent = std::dynamic_pointer_cast<CTimingGameEvent>(spEvent))
		progress = spTimingGameEvent->m_Score;
	else if (CWarDiceEvent::SharedPtr spWarDiceEvent = std::dynamic_pointer_cast<CWarDiceEvent>(spEvent))
		progress = spWarDiceEvent->m_DiceBlock;
	else
		return false;

	INT32 round = spEvent->m_Round;
	INT32 prevRewardKind = spEvent->m_RewardKind;

	////////// 인메모리 선처리 //////////
	spEvent->m_RewardKind = rewardKind;
	//////////////////////////////////// 


	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;

	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = round;
			pQuery->m_nProgress = progress;
			pQuery->m_nRewardKind = rewardKind;

			pQuery->TVP_1.clear();
			pQuery->TVP_2.clear();

			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->TVP_3.clear();

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;

			pQuery->TVP_4.clear();

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_SelectRewardKind. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
							rollbackEvent->m_RewardKind = prevRewardKind;

						return RECV_OK;
					}

					if (auto spScheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID))
					{
						switch (spScheduleInfo->EventType)
						{
							case CustomEvent::eEventType::BINGO:
							{
								GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_SELECT, 0, 0,
									{ eventID, rewardKind, rewardItemCount, round }, {});
							}
							break;
							case CustomEvent::eEventType::ROULETTE:
							{
								GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_SELECT, 0, 0,
									{ eventID, rewardKind, rewardItemCount, round }, {});
							}
							break;
							case CustomEvent::eEventType::TIMINGGAME:
							{
								GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD_SELECT, 0, 0,
									{ eventID, rewardKind, rewardItemCount, round }, {});
							}
							break;
							case CustomEvent::eEventType::WARDICE:
							{
								GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_SELECT, 0, 0,
									{ eventID, rewardKind, rewardItemCount, round }, {});
							}
							break;
						}
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK(fbb, eventID, rewardKind);
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_SelectRewardKind. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}


bool CustomEventHelper::Send_GS_CUSTOM_EVENT_INFO_NFY(CUser* const pUser, const INT64 eventID)
{
	if (false == IS_ACTIVE_USER(pUser))
		return false;

	NEW_FLATBUFFER(GS_CUSTOM_EVENT_INFO_NFY, pPACKET);
	pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_SCHEDULE>>			vecEventInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_ROUND>>				vecRoundInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_PROBABILITY>>		vecProbabilityInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_QUEST>>				vecQuestInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_VALUE_INFO>>		vecValueInfos;
		
		auto& curEventMap = ASE_INSTANCE(pUser, CCustomEventContainer)->GetEventRepository();
		for (const auto& pair_event : curEventMap)
		{
			auto& pEvent = (pair_event).second->pEvent;
			if (pEvent == nullptr || pEvent->m_State != CustomEvent::eEventState::PROCESSING)
				continue;

			if (eventID > 0 && eventID != pEvent->m_EventID)
				continue;

			CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(pEvent->m_EventID);
			if (pEventInfo == nullptr)
				continue;
			
			std::vector<INT32> vecProgress;

			if (auto pBingoEvent = std::dynamic_pointer_cast<CBingoEvent>(pEvent))
				ConvertBingoBoardToVector(pBingoEvent->m_BingoBoard, OUT vecProgress);
			else if (auto pRouletteEvent = std::dynamic_pointer_cast<CRouletteEvent>(pEvent))
				vecProgress.emplace_back(pRouletteEvent->m_Score);
			else if(auto pTimingGameEvent = std::dynamic_pointer_cast<CTimingGameEvent>(pEvent))
				vecProgress.emplace_back(pTimingGameEvent->m_Score);
			else if(auto pWarDiceEvent = std::dynamic_pointer_cast<CWarDiceEvent>(pEvent))
				vecProgress.emplace_back(pWarDiceEvent->m_DiceBlock);

			vecEventInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_SCHEDULE(fbb
				, pEvent->m_EventID
				, static_cast<int32_t>(pEventInfo->EventType)
				, pEventInfo->QuestGroup
				, static_cast<int16_t>(pEventInfo->ScheduleType)
				, pEvent->m_StartTime
				, pEvent->m_EndTime
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->IconKey.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->IconSprite.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->IconAtlas.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->TitleKey.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->HelpKey.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->NumberImagePath.c_str())
				, CREATE_FLATBUFFER_RAWSTR(pEventInfo->BGImagePath.c_str())
				, pEventInfo->KeyItemKind
				, pEventInfo->KeyItemCount
				, pEvent->m_RewardKind
				, fbb.CreateVector(vecProgress)
				, pEvent->m_Round
				, pEventInfo->TapInfo
			));

			std::vector<CustomEvent::RoundInfo::SharedPtr> out_vecRound;
			CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfos(pEvent->m_EventID, OUT out_vecRound);
			for (const auto& iter_round : out_vecRound)
			{
				std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_REWARD>>		vecRewardInfos;

				std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
				CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(pEvent->m_EventID, iter_round->Round, pUser->GetNation(), OUT out_vecReward);
				for (const auto& iter_reward : out_vecReward)
				{
					auto pRewardInfo = (std::get<1>(iter_reward)).get();
					if (pRewardInfo == nullptr)
						continue;

					if (pRewardInfo->Nation != GAME::eNATION::eNATION_NONE && pRewardInfo->Nation != pUser->GetLord().GetNation())
						continue;

					CCustumEventReward::SharedPtr pReward = nullptr;
					auto key = std::make_tuple(iter_round->Round, pRewardInfo->RewardType);
					if (pEvent->m_RewardMap.end() != pEvent->m_RewardMap.find(key))
						pReward = pEvent->m_RewardMap.at(key);

					bool bRewarded = false;
					if (pReward != nullptr && pReward->m_RewardValue == 1)
						bRewarded = true;

					std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatRewardItems;
					for (const auto& item : pRewardInfo->RewardItems)
						vecFlatRewardItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, item.Count));

					vecRewardInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_REWARD(fbb
						, pEvent->m_EventID
						, pRewardInfo->RewardGroupID
						, pRewardInfo->RewardType
						, fbb.CreateVector(vecFlatRewardItems)
						, bRewarded
					));
				}
				
				vecRoundInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_ROUND(fbb
					, pEvent->m_EventID
					, iter_round->Round
					, fbb.CreateVector(iter_round->RoundGoal)
					, fbb.CreateVector(vecRewardInfos)
					, CREATE_FLATBUFFER_RAWSTR(iter_round->RoundImagePath.c_str())
				));
			}

			std::vector<CustomEvent::ProbabilityInfo::SharedPtr> out_vecProbability;
			CustomEvent::CCustomEventInfoManager::Instance()->GetProbabilityInfos(pEvent->m_EventID, OUT out_vecProbability);
			for (const auto& iter_probability : out_vecProbability)
			{
				vecProbabilityInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_PROBABILITY(fbb
					, pEvent->m_EventID
					, iter_probability->ProbKey_1
					, iter_probability->ProbKey_2
				));
			}

			auto questMap = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfos(pEventInfo->QuestGroup);
			if (questMap != nullptr)
			{
				for (const auto& [_, pQuestInfo] : *questMap)
				{
					if (pQuestInfo == nullptr)
						continue;

					CCustomEventQuest::SharedPtr pQuest = nullptr;
					if (pEvent->m_QuestMap.end() != pEvent->m_QuestMap.find(pQuestInfo->QuestID))
						pQuest = pEvent->m_QuestMap.at(pQuestInfo->QuestID);

					std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatRewardItems;
					for (const auto& item : pQuestInfo->RewardItems)
						vecFlatRewardItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, item.Count));

					vecQuestInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_QUEST(fbb
						, pEvent->m_EventID
						, pQuestInfo->QuestID
						, pQuestInfo->ConditionKind
						, pQuestInfo->ConditionValue
						, (pQuest != nullptr) ? pQuest->m_CurValue : 0
						, pQuestInfo->TargetValue
						, CREATE_FLATBUFFER_RAWSTR(pQuestInfo->PointHelpKey.c_str())
						, (pQuest != nullptr ) ? static_cast<Int16>(pQuest->m_MissionState) : 0
						, fbb.CreateVector(vecFlatRewardItems)
					));

				}
			}

			auto valueMap = CustomEvent::CCustomEventInfoManager::Instance()->GetValueInfos(pEvent->m_EventID);
			if (valueMap != nullptr)
			{
				for (const auto& [_, value] : *valueMap)
				{
					vecValueInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_VALUE_INFO(fbb
						, pEvent->m_EventID
						, value->Difficulty
						, value->Value1
						, value->Value2
						, value->Value3
						, value->Value4
						, value->Value5
						, value->Value6
						, value->Value7
						, value->Value8
						, value->Value9
						, value->Value10
					));
				}
			}

		}

		return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_INFO_NFY(fbb
			, fbb.CreateVector(vecEventInfos)
			, fbb.CreateVector(vecRoundInfos)
			, fbb.CreateVector(vecProbabilityInfos)
			, fbb.CreateVector(vecQuestInfos)
			, fbb.CreateVector(vecValueInfos)
		);
	});
	SEND_ACTIVE_USER(pUser, pPACKET);
	return true;
}


bool CustomEventHelper::OnRecv_GS_CUSTOM_EVENT_PLAY_REQ_PlayTimingGame(CUser* const pUser, const INT64 eventID, const UINT8 difficulty)
{
	if (pUser == nullptr)
		return false;

	CCustumEventBase::SharedPtr pEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (pEvent == nullptr || pEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	CTimingGameEvent::SharedPtr pTimingGame = std::dynamic_pointer_cast<CTimingGameEvent>(pEvent);
	if (pTimingGame == nullptr)
		return false;

	pTimingGame->ClearProgress();
	
	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < pEvent->m_StartTime || pEvent->m_EndTime < curTime)
		return false;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return false;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, pEvent->m_Round);
	if (pRoundInfo == nullptr)
		return false;

	CustomEvent::eCustomEventDifficulty reqDifficulty = static_cast<CustomEvent::eCustomEventDifficulty>(difficulty);

	INT32 keyItemKind = pEventInfo->KeyItemKind;
	INT64 keyItemCount = static_cast<INT64>(pEventInfo->KeyItemCount * difficulty);

	BASE::ITEM_INFO* itemInfo = BASE::GET_ITEM_DATA(keyItemKind);
	if (itemInfo == nullptr || keyItemCount <= 0)
		return false;

	// 재료 아이템이 충분한지 검사
	if (false == pUser->GetInventory().IsItemEnough(keyItemKind, keyItemCount))
		return false;

	pUser->GetInventory().AddItemNum(keyItemKind, -keyItemCount);

	//아이템 삭제..
	::OnRemoteDBA([=, user_id = pUser->UID()](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_ITEM_DELETE_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->TVP_1.clear();

			pQuery->TVP_1.m_nUserID.push_back(user_id);
			pQuery->TVP_1.m_nItemKind.push_back(keyItemKind);
			pQuery->TVP_1.m_nItemCount_Sub.push_back(keyItemCount);


			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				Int32 nResult = (false == SET_1.empty()) ? SET_1[0].m_nResult : -1;

				auto user_object = CUserManager::Instance()->GetByUID(user_id);
				if (!user_object)
				{
					LOGGER_ERROR(CONST_EVENT_LOG,
						"Error: Can't Find UserObject, userID:{} P_ITEM_DELETE_TV Result : {}", user_id, nResult);

					return RECV_OK;
				}





				if (0 != nResult)
				{
					// 이상해!
					LOGGER_ERROR(CONST_EVENT_LOG,
						"Error:  - userID:{}, P_ITEM_DELETE_TV Result : {}", user_id, nResult);

					if (user_object)
						user_object->GetInventory().AddItemNum(keyItemKind, keyItemCount);
					return RECV_OK;
				}


				//디비 잘 갔다 왔으면 클라로 나머지 처리..!
				auto successbarSize = CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameSuccessBarSize(eventID, reqDifficulty);
				auto greatSuccessbarSize = CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameGreatSuccessBarSize(eventID, reqDifficulty);

				auto halfSuccesbarSize = successbarSize / 2;
				auto halfgreatSuccessbarSize = greatSuccessbarSize / 2;

				INT32 successPosX = CRandManager::RAND(EnumRandType::EVENT_BASE, halfSuccesbarSize, 10000 - halfSuccesbarSize);
				if (successPosX > 10000)
					return RECV_OK;

				INT32 successLeftX     = successPosX - halfSuccesbarSize;
				INT32 SuccessRightX    = successPosX + halfSuccesbarSize;
				INT32 GreatSuccessPosX = CRandManager::RAND(EnumRandType::EVENT_BASE, successLeftX + halfgreatSuccessbarSize, SuccessRightX - halfgreatSuccessbarSize);


				//플레이 로그
				GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_PLAY, 0, 0,
					{ eventID, difficulty }, {});

				auto pTimingGame = std::dynamic_pointer_cast<CTimingGameEvent>(ASE_INSTANCE(user_object, CCustomEventContainer)->Seek(eventID));
				if  (pTimingGame)
				{
					pTimingGame->SetProgress(successPosX, GreatSuccessPosX, 0, 0);

					if (IS_ACTIVE_USER(pUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
							{

								auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, 0, 0, 0, 0);
								auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pUser->GetAssets().GetOil(), pUser->GetAssets().GetIron(), pUser->GetAssets().GetSilver(), pUser->GetAssets().GetGold());

								return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, keyItemKind, keyItemCount, 0, pTimingGame->m_Score, successPosX, GreatSuccessPosX, pTimingGame->m_Round, difficulty, 0, 0,
									sendAddAsset, curAsset, 0, 0, 0, 0);
							});
						SEND_ACTIVE_USER(pUser, pPACKET);
					}
				}
				else
				{
					LOGGER_ERROR(CONST_EVENT_LOG, "Error: Can't Find TimingGame, userID:{} EventID:{}", user_id, eventID);
					return RECV_OK;
				}

				
				return RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});

	return true;
}

bool CustomEventHelper::OnRecvGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(CUser* pUser, const void* pData)
{
	if (pUser == nullptr)
		return false;

	const auto pReq = PROTOCOL::FLATBUFFERS::GetGS_CUSTOM_EVENT_TIMINGGAME_PLAY_REQ(pData);
	if (pReq == nullptr)
		return false;

	CustomEvent::eCustomEventDifficulty reqDifficulty = static_cast<CustomEvent::eCustomEventDifficulty>(pReq->Difficulty());
	INT64 reqEventID = pReq->EventID();
	bool  reqTimeOut = pReq->TimeOut();
	
	CCustumEventBase::SharedPtr pEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(reqEventID);
	if (pEvent == nullptr || pEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	CTimingGameEvent::SharedPtr pTimingGame = std::dynamic_pointer_cast<CTimingGameEvent>(pEvent);
	if (pTimingGame == nullptr)
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < pEvent->m_StartTime || pEvent->m_EndTime < curTime)
		return false;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(reqEventID);
	if (pEventInfo == nullptr)
		return false;

	INT32 serverID = GLOBAL::GS_INFO.SVID;
	INT64 userID = pUser->UID();

	auto successbarSize = CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameSuccessBarSize(reqEventID, reqDifficulty);
	auto greatSuccessbarSize = CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameGreatSuccessBarSize(reqEventID, reqDifficulty);

	auto halfSuccesbarSize = successbarSize / 2;
	auto halfgreatSuccessbarSize = greatSuccessbarSize / 2;

	INT32 addItemCount = 0;
	auto userPoint = pReq->ArrowPoint();

	INT32 prevSuccessX = pTimingGame->m_SuccessBarX;
	INT32 prevGreatSuccessX = pTimingGame->m_GreatSuccessBarX;

	INT32 prevSuccessLeftX = prevSuccessX - halfSuccesbarSize;
	INT32 prevSuccessRightX = prevSuccessX + halfSuccesbarSize;

	INT32 prevGreatSuccessLeftX = prevGreatSuccessX - halfgreatSuccessbarSize;
	INT32 prevGreatSuccessRightX = prevGreatSuccessX + halfgreatSuccessbarSize;

	//대성공
	if (reqTimeOut == false)
	{
		if (userPoint >= prevGreatSuccessLeftX && userPoint <= prevGreatSuccessRightX)
		{
			addItemCount += CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameGreatSuccessAddItemCount(reqEventID, reqDifficulty);
			GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::CUSTOM_EVENT_TIMING_GAME_ON_PERFECT, 0, 1);
		}
		else if (userPoint >= prevSuccessLeftX && userPoint <= prevSuccessRightX)
			addItemCount += CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameSuccessAddItemCount(reqEventID, reqDifficulty);
		else
			addItemCount += CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameFailAddItemCount(reqEventID, reqDifficulty);;
	}

	INT32 successPosX = CRandManager::RAND(EnumRandType::EVENT_BASE, halfSuccesbarSize, 10000 - halfSuccesbarSize);

	if (successPosX > 10000)
		return false;

	INT32 successLeftX = successPosX - halfSuccesbarSize;
	INT32 successRightX = successPosX + halfSuccesbarSize;
	INT32 GreatSuccessPosX = CRandManager::RAND(EnumRandType::EVENT_BASE, successLeftX + halfgreatSuccessbarSize, successRightX - halfgreatSuccessbarSize);

	INT32 timingGameRound = pTimingGame->m_Round;
	INT32 curBonusPoint = pTimingGame->m_BounsPoint += addItemCount;
	pTimingGame->SetProgress(successPosX, GreatSuccessPosX, ++timingGameRound, curBonusPoint);

	//이게 마지막 발이다..!!!
	if (timingGameRound == CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameTotalCount(reqEventID, reqDifficulty) || reqTimeOut == true)
	{
		//이번에 획득한 총 점수
		auto totalAddItemCount = pTimingGame->m_BounsPoint;

		//원래 내 점수 + 이번 점수
		INT32 afterProgress = pTimingGame->m_Score + totalAddItemCount;

		//원래 내 점수
		INT32 prevProgress = pTimingGame->m_Score;

		//원래 내 라운드
		INT32 prevRound = pEvent->m_Round;

		//원래 내 보상
		INT32 prevRewardKind = pEvent->m_RewardKind;

		//점수 선처리
		pTimingGame->m_Score = afterProgress;


		std::set<std::tuple<INT32, INT32>> rewardTypes;
		std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
		CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(pEvent->m_EventID, -1, pUser->GetNation(), out_vecReward);

		//보상 정보 수집
		for (const auto& iter_reward : out_vecReward)
		{
			INT32 round = std::get<0>(iter_reward);
			auto pRewardInfo = (std::get<1>(iter_reward)).get();
			if (pRewardInfo == nullptr)
				continue;

			CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(reqEventID, round);
			if (pRoundInfo == nullptr)
				continue;

			if (pRewardInfo->RewardType > 0)
			{
				if (pRoundInfo->RoundGoal.size() < pRewardInfo->RewardType)
					continue;

				INT32 rewardIdx = pRewardInfo->RewardType - 1;
				INT32 compareScore = pRoundInfo->RoundGoal[rewardIdx];
				if (prevProgress < compareScore && compareScore <= afterProgress)
				{
					if (false == rewardTypes.emplace(std::make_tuple(round, pRewardInfo->RewardType)).second)
						continue;
				}
			}
			else
			{
				// 현재 라운드가 아니라면 최종 보상은 받을 수 없다
				if (prevRound != round)
					continue;

				// 해당 라운드 최종 스코어에 도달했다면 라운드 보상 추가 지급
				INT32 finalScore = pRoundInfo->RoundGoal[pRoundInfo->RoundGoal.size() - 1];
				if (prevProgress < finalScore && finalScore <= afterProgress)
				{
					if (false == rewardTypes.emplace(std::make_tuple(round, 0)).second)
						continue;
				}
			}
		}

		// Valid Check
		for (const auto& tuple : rewardTypes)
		{
			INT32 round = std::get<0>(tuple);
			INT32 rewardType = std::get<1>(tuple);

			// 현재 라운드거나 다음 라운드까지만 획득 가능
			if (round < pEvent->m_Round || pEvent->m_Round + 1 < round)
				return false;

			if (pEvent->IsRewarded(round, rewardType))
				return false;

			// 라운드 최종 보상의 경우 현재 라운드 보상이 아니라면 받을수 없다
			if (rewardType == 0 && round != pEvent->m_Round)
				return false;

			CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(reqEventID, round);
			if (pRewardRoundInfo == nullptr)
				continue;

			INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

			CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
			if (pRewardInfo == nullptr)
				pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
			if (pRewardInfo == nullptr)
				continue;

			for (const auto& reward : pRewardInfo->RewardItems)
			{
				// 라운드 Complete 보상은 선택한 보상만 받는다
				if (rewardType == 0 && pEvent->m_RewardKind != reward.Kind)
					continue;

				if (false == EventHelper::CheckValidItem(reward.Kind, curTime))
					return false;
			}
		}

		

		std::vector<INT32> prevRewardTypes;
		pEvent->GetReward(prevRound, OUT prevRewardTypes);

		

		INT32 afterRound = -1;
		for (const auto& tuple : rewardTypes)
		{
			INT32 round = std::get<0>(tuple);
			INT32 rewardType = std::get<1>(tuple);

			pEvent->SetReward(round, rewardType, 1);

			if (rewardType > 0)
				afterRound = std::max<INT32>(afterRound, round);
			else
				afterRound = std::max<INT32>(afterRound, round + 1);
		}

		if (-1 < afterRound && afterRound != pEvent->m_Round && prevRound < pEventInfo->MaxRound && afterRound <= pEventInfo->MaxRound)
		{
			pEvent->m_Round = afterRound;
			pEvent->m_RewardKind = 0;
		}
		else
		{
			afterRound = pEvent->m_Round;
		}

		INT32 afterRewardKind = pEvent->m_RewardKind;

		std::vector<INT32> afterRewardTypes;
		pEvent->GetReward(afterRound, OUT afterRewardTypes);
		

		timingGameRound = reqTimeOut == true ? CustomEvent::CCustomEventInfoManager::Instance()->GetTimingGameTotalCount(reqEventID, reqDifficulty) : timingGameRound;
				
		INT64 AddOil = 0;
		INT64 AddIron = 0;
		INT64 AddSilver = 0;
		INT64 AddGold = 0;
		INT64 AddGem = 0;
		INT64 AddFreeGem = 0;

		std::vector<BASE::REWARDITEM> vecAddItems;
		std::vector<BASE::COUPON>	vecCoupons;

		for (const auto& tuple : rewardTypes)
		{
			INT32 round = std::get<0>(tuple);
			INT32 rewardType = std::get<1>(tuple);

			CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(reqEventID, round);
			if (pRewardRoundInfo == nullptr)
				continue;

			INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

			CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
			if (pRewardInfo == nullptr)
				pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
			if (pRewardInfo == nullptr)
				continue;

			for (const auto& reward : pRewardInfo->RewardItems)
			{
				// 라운드 Complete 보상은 선택한 보상만 받는다
				if (rewardType == 0 && prevRewardKind != reward.Kind)
					continue;

				auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
				if (iteminfo == nullptr)
					continue;

				if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
				{
					if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL)		    AddOil += reward.Count;
					else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON)	    AddIron += reward.Count;
					else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER)	AddSilver += reward.Count;
					else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD)	    AddGold += reward.Count;
				}
				else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE)	// 젬
				{
					if (iteminfo->i32VALUE[3] == 0) AddGem += iteminfo->i32VALUE[1] * reward.Count;	// 유가
					else							AddFreeGem += iteminfo->i32VALUE[1] * reward.Count;	// 무가
				}
				else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_COUPON)		// 쿠폰
				{
					int64_t	nowTime = GetDueDay_UTC(0);
					int64_t	endTime = GetDueDay_UTC(iteminfo->i32VALUE[2]);

					if (GAME::eITEM_USE_TYPE::COUPON_INFINITY == iteminfo->i32ITEM_USE_TYPE)
					{
						endTime = GetDueDay_UTC(static_cast<INT64>(SECOND_PER_DAY) * 30LL * 12LL * 100LL);	// 100년 뒤
					}

					for (int iCnt = 0; iCnt < reward.Count; iCnt++)
					{
						auto couponData = BASE::COUPON_INFO_DATA.Get(iteminfo->i32ITEM_KIND);
						wstring strProductKinds = (nullptr == couponData) ? L"" : couponData->strPackageKind;
						vecCoupons.emplace_back(BASE::COUPON(0, 1, iteminfo->i32ITEM_KIND, nowTime, endTime, iteminfo->i32VALUE[1],
							strProductKinds));
					}
				}
				else	// 일반 아이템일때는 중복 체크하고 적재
				{
					bool isSet = false;
					for (int k = 0; k < vecAddItems.size(); ++k)
					{
						if (vecAddItems[k].Kind == reward.Kind)
						{
							vecAddItems[k].Count += reward.Count;
							isSet = true;
						}
					}
					if (false == isSet)
						vecAddItems.emplace_back(BASE::REWARDITEM(reward.Kind, reward.Count));
				}
			}
		}
		

		::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
			{
				BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
				pQuery->m_nServerID = serverID;
				pQuery->m_nUserID = userID;
				pQuery->m_nEventID = reqEventID;
				pQuery->m_nRound = afterRound;
				pQuery->m_nProgress = afterProgress;
				pQuery->m_nRewardKind = afterRewardKind;

				pQuery->m_nAddOil = AddOil;
				pQuery->m_nAddIron = AddIron;
				pQuery->m_nAddSilver = AddSilver;
				pQuery->m_nAddGold = AddGold;
				pQuery->m_nAddGem = AddGem;
				pQuery->m_nAddFreeGem = AddFreeGem;

				pQuery->TVP_1.clear();
				for (const auto& rewardType : afterRewardTypes)
				{
					pQuery->TVP_1.m_nKey.push_back(rewardType);
					pQuery->TVP_1.m_nValue.push_back(1);
				}

				if (afterRewardTypes.size() <= 0)
				{
					pQuery->TVP_1.m_nKey.push_back(1);
					pQuery->TVP_1.m_nValue.push_back(0);
				}

				//타이밍 게임은 시작할 때 이미 아이템 삭제했음.
				pQuery->TVP_2.clear();
				/*for (const auto& item : vecDeleteItems)
				{
					pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
					pQuery->TVP_2.m_nItemCount.push_back(item.Count);
					pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
				}*/

				pQuery->TVP_3.clear();
				for (const auto& item : vecAddItems)
				{
					pQuery->TVP_3.m_nItemKind.push_back(item.Kind);
					pQuery->TVP_3.m_nItemCount.push_back(item.Count);
					pQuery->TVP_3.m_nItemCount_Svr.push_back(0);
				}

				pQuery->TVP_4.clear();
				for (const auto& coupon : vecCoupons)
				{
					pQuery->TVP_4.m_nItemKind.push_back(coupon.ItemKind);
					pQuery->TVP_4.m_nUniqueID.push_back(coupon.UniqueID);
					pQuery->TVP_4.m_nisActive.push_back(coupon.isActive);
					pQuery->TVP_4.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
					pQuery->TVP_4.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
					pQuery->TVP_4.m_nDiscountRate.push_back(coupon.DiscountRate);

					NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
					pQuery->TVP_4.m_nProductKinds.push_back(strProductKinds);
					pQuery->TVP_4.m_nProductKindsLen.push_back(static_cast<Int64>(strProductKinds.GetLength()) * static_cast<Int64>(strProductKinds.GetCharTypeSize()));
				}


				RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

				::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
				{
					if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
					{
						if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
						{
							if (SET_1.size() > 0)
								LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_REWARD_SET. Fail Result:{}", SET_1[0].m_nResult);

							CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(reqEventID);
							if (rollbackEvent != nullptr)
							{
								if (auto rollbackTimingGame = std::dynamic_pointer_cast<CTimingGameEvent>(rollbackEvent))
									rollbackTimingGame->m_Score = prevProgress;

								rollbackEvent->m_RewardMap.clear();
								for (const auto& rewardType : prevRewardTypes)
									rollbackEvent->SetReward(prevRound, rewardType, 0);

								rollbackEvent->m_Round = prevRound;
								rollbackEvent->m_RewardKind = prevRewardKind;
							}

							return RECV_OK;
						}

						GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::CUSTOM_EVENT_TIMING_GAME_CLEAR, static_cast<INT64>(reqDifficulty), 1);

						// 자원 획득 후처리
						PROTOCOL::ASSET addAsset;
						if (AddOil > 0)
						{
							INT64 prevNum = pResultUser->GetAssets().GetOil();
							addAsset.AddOil(AddOil);
							INT64 aftrNum = pResultUser->GetAssets().GetOil();
							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL, prevNum, aftrNum }, {});
						}
						if (AddIron > 0)
						{
							INT64 prevNum = pResultUser->GetAssets().GetIron();
							addAsset.AddIron(AddIron);
							INT64 aftrNum = pResultUser->GetAssets().GetIron();
							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON, prevNum, aftrNum }, {});
						}
						if (AddSilver > 0)
						{
							INT64 prevNum = pResultUser->GetAssets().GetSilver();
							addAsset.AddSilver(AddSilver);
							INT64 aftrNum = pResultUser->GetAssets().GetSilver();
							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER, prevNum, aftrNum }, {});
						}
						if (AddGold > 0)
						{
							INT64 prevNum = pResultUser->GetAssets().GetGold();
							addAsset.AddGold(AddGold);
							INT64 aftrNum = pResultUser->GetAssets().GetGold();

							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD, prevNum, aftrNum }, {});
						}

						if (AddGem > 0)
						{
							/*GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::VC_GEM_KIND, 0, AddGem }, {});*/


							/*GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
								{ eventID, prevRound, afterRound, addScore, GAME::VC_GEM_KIND, 0, AddGem, 0 }, {});*/
						}

						if (AddFreeGem > 0)
						{
							/*GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, GAME::FREE_GEM_KIND, 0, AddFreeGem }, {});*/
							/*GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
								{ eventID, prevRound, afterRound, addScore, GAME::FREE_GEM_KIND, 0, AddFreeGem, 0 }, {});*/
						}

						pResultUser->PublicLog_Contents_SendAddGem(ePublicLogReason::REASON_ADD_GEM_TIMING_EVENT_REWARD, 0, AddGem, AddFreeGem);

						pResultUser->GetAssets().AddAssets(addAsset);

						std::unordered_map<INT32, INT32> mapDirectUse;
						std::vector<INT32> newItemKinds;
						for (const auto& item : vecAddItems)
						{
							INT64 prevNum = pResultUser->GetInventory().GetItemNum(item.Kind);
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
							newItemKinds.push_back(item.Kind);

							auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
							if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
							{
								if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
									mapDirectUse[item.Kind] += item.Count;
								else
									mapDirectUse.emplace(item.Kind, item.Count);
							}

							INT64 resultNum = pResultUser->GetInventory().GetItemNum(item.Kind);

							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, item.Kind, prevNum, resultNum }, {});
						}

						for (const auto& tuple : rewardTypes)
						{
							INT32 round = std::get<0>(tuple);
							INT32 rewardType = std::get<1>(tuple);

							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD_TYPE, 0, 0,
								{ reqEventID, prevRound, afterRound, totalAddItemCount, round, rewardType }, {});
						}

						/*for (const auto& coupon : vecCoupons)
						{

							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ reqEventID, static_cast<INT64>(reqDifficulty), 0, totalAddItemCount, coupon.ItemKind, 0, AddFreeGem }, {});

							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_REWARD, 0, 0,
								{ eventID, prevRound, afterRound, addScore, coupon.ItemKind, 0, 1, 1 }, {});
						}*/

						// 쿠폰 획득 로그
						for (const auto& coupon : repo_2)
						{
							auto couponBeginTime = coupon.m_tmBeginTime;
							auto couponEndTime = coupon.m_tmEndTime;
							GLOBAL::SendLog(userID, 0, DB_LOG::REASON_COUPON_GET, 0, 0, { coupon.m_nUniqueID, coupon.m_nItemKind, 0, 1, 0,
								static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)), static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)) }, {});
						}

						if (newItemKinds.size() > 0)
						{
							NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
							pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
							{
								return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
							});
							SEND_DBA(pNEWITEMPACKET);
						}

						if (IS_ACTIVE_USER(pResultUser))
						{
							NEW_FLATBUFFER(GS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK, pPACKET);
							pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
							{
								std::vector<INT32> vecFlatRewardIDs;
								for (const auto& rewardID : afterRewardTypes)
									vecFlatRewardIDs.emplace_back(rewardID);

								std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::ITEM>> vecFlatAddItems;
								for (const auto& item : vecAddItems)
									vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateITEM(fbb, item.Kind, pResultUser->GetInventory().GetItemNum(item.Kind), 0, 0, 0, 0));


								std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
								for (const auto& coupon : repo_2)
								{
									auto couponBeginTime = coupon.m_tmBeginTime;
									auto couponEndTime = coupon.m_tmEndTime;
									vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
										static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
										static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
										::to_flatbuffer(fbb, coupon.m_szProductKinds)));
								}

								auto AddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, AddOil, AddIron, AddSilver, AddGold);

								return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK(fbb,
									reqEventID,
									static_cast<INT16>(reqDifficulty),
									timingGameRound,
									0,
									0,
									addItemCount,
									curBonusPoint,
									fbb.CreateVector(vecFlatAddItems),
									AddAsset,
									AddGem,
									AddFreeGem,
									fbb.CreateVector(vecFlatCoupons),
									fbb.CreateVector(vecFlatRewardIDs),
									afterRound
								);
							});
							SEND_ACTIVE_USER(pResultUser, pPACKET);
						}

						if (mapDirectUse.size() > 0)
						{
							for (auto& itemDU : mapDirectUse)
							{
								auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
								ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
							}
						}
					}
					else
					{
						LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_REWARD_SET. not found user. userid:{}", userID);
					}
					return RECV_OK;
				});

				END_GDB_QUERY();

				return RECV_OK;
			});
	}
	else
	{
		if (IS_ACTIVE_USER(pUser))
		{
			NEW_FLATBUFFER(GS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK, pPACKET);
			pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_TIMINGGAME_PLAY_ACK(fbb,
					pReq->EventID(),
					pReq->Difficulty(),
					timingGameRound,
					successPosX,
					GreatSuccessPosX, 
					addItemCount,
					curBonusPoint,
					0,
					0,
					0,
					0,
					0,
					0,
					0);
			});
			SEND_ACTIVE_USER(pUser, pPACKET);
		}
	}

	return false;
}


void CustomEventHelper::OnRecvGS_CUSTOM_EVENT_MISSION_REWARD_REQ(CUser* pUser, const void* pData)
{
	const auto pReq = PROTOCOL::FLATBUFFERS::GetGS_CUSTOM_EVENT_MISSION_REWARD_REQ(pData);
	if (pReq == nullptr)
		return;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(pReq->EventID());
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return;

	CCustomEventQuest::SharedPtr pQuest = nullptr;
	if (spEvent->m_QuestMap.end() != spEvent->m_QuestMap.find(pReq->QuestID()))
		pQuest = spEvent->m_QuestMap.at(pReq->QuestID());
	
	if (pQuest == nullptr)
		return;

	if (CustomEvent::eCustomEventMissionState::COMPLETE != pQuest->m_MissionState)
		return;

	std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecNeedUpdate;
	vecNeedUpdate.emplace_back(std::make_tuple(spEvent->m_EventID, pQuest->m_QuestID, pQuest->m_CurValue, CustomEvent::eCustomEventMissionState::REWARDED));

	CustomEventHelper::P_CUSTOMEEVENT_QUEST_SET_TV(pUser, vecNeedUpdate, true);
}

bool CustomEventHelper::ConvertBingoBoardFromBitFlag(const INT32 progress, OUT BingoBoardArray& bingoBoard)
{
	for (int c = 0; c < CustomEvent::MAX_BINGO_NUM; ++c)
	{
		INT32 rowIdx = c / CustomEvent::BINGO_ROW_COUNT;
		INT32 colIdx = c % CustomEvent::BINGO_ROW_COUNT;

		bingoBoard[rowIdx][colIdx] = ((1 << c) & progress);
	}

	return true;
}

bool CustomEventHelper::ConvertBingoBoardToBitFlag(const BingoBoardArray& bingoBoard, OUT INT32& progress)
{
	progress = 0;
	for (INT32 rowIdx = 0; rowIdx < CustomEvent::BINGO_ROW_COUNT; rowIdx++)
	{
		for (INT32 colIdx = 0; colIdx < CustomEvent::BINGO_ROW_COUNT; colIdx++)
		{
			if (bingoBoard[rowIdx][colIdx])
				progress |= (1 << (rowIdx * CustomEvent::BINGO_ROW_COUNT + colIdx));
		}
	}

	return true;
}

bool CustomEventHelper::ConvertBingoBoardToVector(const BingoBoardArray& bingoBoard, OUT std::vector<INT32>& vecBingoNum)
{
	vecBingoNum.clear();
	for (INT32 rowIdx = 0; rowIdx < CustomEvent::BINGO_ROW_COUNT; rowIdx++)
	{
		for (INT32 colIdx = 0; colIdx < CustomEvent::BINGO_ROW_COUNT; colIdx++)
		{
			if (bingoBoard[rowIdx][colIdx])
				vecBingoNum.emplace_back(rowIdx * CustomEvent::BINGO_ROW_COUNT + colIdx);
		}
	}

	return true;
}

bool CustomEventHelper::CheckComplete(const BingoBoardArray& bingoBoard)
{
	for (INT32 rowIdx = 0; rowIdx < CustomEvent::BINGO_ROW_COUNT; rowIdx++)
	{
		for (INT32 colIdx = 0; colIdx < CustomEvent::BINGO_ROW_COUNT; colIdx++)
		{
			if (false == bingoBoard[rowIdx][colIdx])
				return false;
		}
	}

	return true;
}

bool CustomEventHelper::CheckBingoCount(const BingoBoardArray& bingoBoard, const INT32 bingoCount)
{
	INT32 curCount = 0;
	for (INT32 rowIdx = 0; rowIdx < CustomEvent::BINGO_ROW_COUNT; rowIdx++)
	{
		for (INT32 colIdx = 0; colIdx < CustomEvent::BINGO_ROW_COUNT; colIdx++)
		{
			if (bingoBoard[rowIdx][colIdx])
				curCount++;
		}
	}

	return (curCount == bingoCount);
}

INT32 CustomEventHelper::GetRandomBingoNumber(const BingoBoardArray& bingoBoard, OUT std::set<INT32>& rewardTypes)
{
	rewardTypes.clear();

	////////////////////////////////
	// 비어있는 빙고 타일 수집
	std::vector<INT32> shuffleVec;
	for (int r = 0; r < CustomEvent::BINGO_ROW_COUNT; r++)
	{
		for (int c = 0; c < CustomEvent::BINGO_ROW_COUNT; c++)
		{
			if (false == bingoBoard[r][c])
				shuffleVec.emplace_back(r * CustomEvent::BINGO_ROW_COUNT + c);
		}
	}

	if (shuffleVec.size() <= 0)
		return -1;

	// Shuffle!!
	std::shuffle(shuffleVec.begin(), shuffleVec.end(), CRandManager::GetDevice(EnumRandType::EVENT_BASE));

	INT32 bingoNum = shuffleVec[0];
	INT32 rowIdx = bingoNum / CustomEvent::BINGO_ROW_COUNT;
	INT32 colIdx = bingoNum % CustomEvent::BINGO_ROW_COUNT;

	////////////////////////////////
	// 수평 빙고 검사
	bool horizontalBingo = true;
	for (int c = 0; c < CustomEvent::BINGO_ROW_COUNT; ++c)
	{
		if (c == colIdx)
		{
			continue;
		}
		else if (false == bingoBoard[rowIdx][c])
		{
			horizontalBingo = false;
			break;
		}
	}

	if (horizontalBingo && false == rewardTypes.emplace(rowIdx + 1).second)
		return -1;

	////////////////////////////////
	// 수직 빙고 검사
	bool verticalBingo = true;
	for (int r = 0; r < CustomEvent::BINGO_ROW_COUNT; ++r)
	{
		if (r == rowIdx)
		{
			continue;
		}
		else if (false == bingoBoard[r][colIdx])
		{
			verticalBingo = false;
			break;
		}
	}

	if (verticalBingo && false == rewardTypes.emplace(colIdx + 6).second)
		return -1;

	////////////////////////////////
	// 11번 대각선 검사
	bool containCurBingo = false;
	bool diagonalBingo = true;
	for (int i = 0; i < CustomEvent::BINGO_ROW_COUNT; ++i)
	{
		int r = i;
		int c = CustomEvent::BINGO_ROW_COUNT - i - 1;

		if (r == rowIdx && c == colIdx)
		{
			containCurBingo = true;
			continue;
		}
		else if (false == bingoBoard[r][c])
		{
			diagonalBingo = false;
			break;
		}
	}

	// containCurBingo: 이번에 뽑은 빙고가 11번 대각선에 포함되어있어야 하며,
	// diagonalBingo  : 11번 대각선 빙고가 완성되어야 emplace.
	if (containCurBingo && diagonalBingo && false == rewardTypes.emplace(11).second)
		return -1;

	////////////////////////////////
	// 12번 대각선 검사
	containCurBingo = false;
	diagonalBingo = true;
	for (int i = 0; i < CustomEvent::BINGO_ROW_COUNT; ++i)
	{
		int r = i;
		int c = i;

		if (r == rowIdx && c == colIdx)
		{
			containCurBingo = true;
			continue;
		}
		else if (false == bingoBoard[r][c])
		{
			diagonalBingo = false;
			break;
		}
	}

	// containCurBingo: 이번에 뽑은 빙고가 12번 대각선에 포함되어있어야 하며,
	// diagonalBingo  : 12번 대각선 빙고가 완성되어야 emplace.
	if (containCurBingo && diagonalBingo && false == rewardTypes.emplace(12).second)
		return -1;

	////////////////////////////////
	// All Complete 검사
	bool allBingo = true;
	for (int r = 0; r < CustomEvent::BINGO_ROW_COUNT; r++)
	{
		for (int c = 0; c < CustomEvent::BINGO_ROW_COUNT; c++)
		{
			if (r == rowIdx && c == colIdx)
			{
				continue;
			}
			if (false == bingoBoard[r][c])
			{
				allBingo = false;
				break;
			}
		}
	}

	if (allBingo && false == rewardTypes.emplace(0).second)
		return -1;

	return bingoNum;
}

bool CustomEventHelper::P_CUSTOM_EVENT_UPDATE_PlayBingo(CUser* const pUser, const INT64 eventID, const INT32 bingoCount)
{
	// Play Bingo

	if (pUser == nullptr)
		return false;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	CBingoEvent::SharedPtr spBingoEvent = std::dynamic_pointer_cast<CBingoEvent>(spEvent);
	if (spBingoEvent == nullptr)
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < spEvent->m_StartTime || spEvent->m_EndTime < curTime)
		return false;

	if (spEvent->m_RewardKind <= 0)
		return false;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return false;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return false;
	
	// All Complete 상태 검사 & 따닥 방지
	if (CheckComplete(spBingoEvent->m_BingoBoard) || !CheckBingoCount(spBingoEvent->m_BingoBoard, bingoCount))
		return false;

	INT32 keyItemKind = pEventInfo->KeyItemKind;
	INT64 keyItemCount = pEventInfo->KeyItemCount;

	BASE::ITEM_INFO* itemInfo = BASE::GET_ITEM_DATA(keyItemKind);
	if (itemInfo == nullptr || keyItemCount <= 0)
		return false;

	// 재료 아이템이 충분한지 검사
	if (false == pUser->GetInventory().IsItemEnough(keyItemKind, keyItemCount))
		return false;

	std::vector<BASE::REWARDITEM> vecDeleteItems;
	vecDeleteItems.emplace_back(BASE::REWARDITEM(keyItemKind, keyItemCount));

	// 이번 빙고 랜덤 뽑기
	std::set<INT32> rewardTypes;
	INT32 bingo = GetRandomBingoNumber(spBingoEvent->m_BingoBoard, OUT rewardTypes);
	if (bingo < 0)
		return false;

	// Valid Check
	for (const auto& rewardType : rewardTypes)
	{
		if (spEvent->IsRewarded(spEvent->m_Round, rewardType))
			return false;

		INT32 groupID = (rewardType == 0) ? pRoundInfo->RoundRewardGroupID : pRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && spEvent->m_RewardKind != reward.Kind)
				continue;

			if (false == EventHelper::CheckValidItem(reward.Kind, curTime))
				return false;
		}
	}

	// 기존 값 backup
	INT32 prevBitFlag = 0;
	ConvertBingoBoardToBitFlag(spBingoEvent->m_BingoBoard, OUT prevBitFlag);
	INT32 prevRound = spEvent->m_Round;
	INT32 prevRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> prevRewardTypes;
	spEvent->GetReward(prevRound, OUT prevRewardTypes);

	////////// 인메모리 선처리 //////////
	spEvent->SetProgress(bingo);

	for (const auto& item : vecDeleteItems)
		pUser->GetInventory().AddItemNum(item.Kind, -item.Count);

	UINT8 isClear = 0;
	for (const auto& rewardType : rewardTypes)
	{
		if (rewardType == 0)
			isClear = 1;

		spEvent->SetReward(spEvent->m_Round, rewardType, 1);
	}

	if (isClear && spEvent->m_Round < pEventInfo->MaxRound)
	{
		spEvent->m_Round += 1;
		spEvent->m_RewardKind = 0;
		spEvent->ClearProgress();
	}
	else
	{
		isClear = 0;
	}
	//////////////////////////////////// 

	INT32 newBitFlag = 0;
	ConvertBingoBoardToBitFlag(spBingoEvent->m_BingoBoard, OUT newBitFlag);
	INT32 afterRound = spEvent->m_Round;
	INT32 afterRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> afterRewardTypes;
	spEvent->GetReward(afterRound, OUT afterRewardTypes);

	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;

	for (const auto& rewardType : rewardTypes)
	{
		INT32 groupID = (rewardType == 0) ? pRoundInfo->RoundRewardGroupID : pRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && prevRewardKind != reward.Kind)
				continue;

			auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
			if (iteminfo == nullptr)
				continue;

			if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
			{
				if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL)		    AddOil += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON)	    AddIron += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER)	AddSilver += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD)	    AddGold += reward.Count;
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE)	// 젬
			{
				if (iteminfo->i32VALUE[3] == 0) AddGem += iteminfo->i32VALUE[1] * reward.Count;	// 유가
				else							AddFreeGem += iteminfo->i32VALUE[1] * reward.Count;	// 무가
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_COUPON)		// 쿠폰
			{
				int64_t	nowTime = GetDueDay_UTC(0);
				int64_t	endTime = GetDueDay_UTC(iteminfo->i32VALUE[2]);

				if (GAME::eITEM_USE_TYPE::COUPON_INFINITY == iteminfo->i32ITEM_USE_TYPE)
				{
					endTime = GetDueDay_UTC(static_cast<INT64>(SECOND_PER_DAY) * 30LL * 12LL * 100LL);	// 100년 뒤
				}

				for (int iCnt = 0; iCnt < reward.Count; iCnt++)
				{
					auto couponData = BASE::COUPON_INFO_DATA.Get(iteminfo->i32ITEM_KIND);
					wstring strProductKinds = (nullptr == couponData) ? L"" : couponData->strPackageKind;
					vecCoupons.emplace_back(BASE::COUPON(0, 1, iteminfo->i32ITEM_KIND, nowTime, endTime, iteminfo->i32VALUE[1],
						strProductKinds));
				}
			}
			else	// 일반 아이템일때는 중복 체크하고 적재
			{
				bool isSet = false;
				for (int k = 0; k < vecAddItems.size(); ++k)
				{
					if (vecAddItems[k].Kind == reward.Kind)
					{
						vecAddItems[k].Count += reward.Count;
						isSet = true;
					}
				}
				if (false == isSet)
					vecAddItems.emplace_back(BASE::REWARDITEM(reward.Kind, reward.Count));
			}
		}
	}

	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = newBitFlag;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();
			for (const auto& item : vecDeleteItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->TVP_3.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_3.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_3.m_nItemCount.push_back(item.Count);
				pQuery->TVP_3.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;

			pQuery->TVP_4.clear();
			for (const auto& coupon : vecCoupons)
			{
				pQuery->TVP_4.m_nItemKind.push_back(coupon.ItemKind);
				pQuery->TVP_4.m_nUniqueID.push_back(coupon.UniqueID);
				pQuery->TVP_4.m_nisActive.push_back(coupon.isActive);
				pQuery->TVP_4.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
				pQuery->TVP_4.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
				pQuery->TVP_4.m_nDiscountRate.push_back(coupon.DiscountRate);

				NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
				pQuery->TVP_4.m_nProductKinds.push_back(strProductKinds);
				pQuery->TVP_4.m_nProductKindsLen.push_back(static_cast<INT64>(strProductKinds.GetLength()) * static_cast<INT64>(strProductKinds.GetCharTypeSize()));
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayBingo. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
						{
							if (auto rollbackBingo = std::dynamic_pointer_cast<CBingoEvent>(rollbackEvent))
								ConvertBingoBoardFromBitFlag(prevBitFlag, OUT rollbackBingo->m_BingoBoard);

							rollbackEvent->m_RewardMap.clear();
							for (const auto& rewardType : prevRewardTypes)
								rollbackEvent->SetReward(prevRound, rewardType, 0);

							rollbackEvent->m_Round = prevRound;
							rollbackEvent->m_RewardKind = prevRewardKind;
						}
						
						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					INT32 bingoNum_log = bingo + 1;

					// 자원 획득 후처리
					PROTOCOL::ASSET addAsset;
					if (AddOil > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetOil();
						addAsset.AddOil(AddOil);
						INT64 aftrNum = pResultUser->GetAssets().GetOil();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL, prevNum, AddOil, aftrNum }, {});
					}
					if (AddIron > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetIron();
						addAsset.AddIron(AddIron);
						INT64 aftrNum = pResultUser->GetAssets().GetIron();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON, prevNum, AddIron, aftrNum }, {});
					}
					if (AddSilver > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetSilver();
						addAsset.AddSilver(AddSilver);
						INT64 aftrNum = pResultUser->GetAssets().GetSilver();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER, prevNum, AddSilver, aftrNum }, {});
					}
					if (AddGold > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetGold();
						addAsset.AddGold(AddGold);
						INT64 aftrNum = pResultUser->GetAssets().GetGold();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD, prevNum, AddGold, aftrNum }, {});
					}

					if (AddGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::VC_GEM_KIND, 0, AddGem, 0 }, {});
					}

					if (AddFreeGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, GAME::FREE_GEM_KIND, 0, AddFreeGem, 0 }, {});
					}

					pResultUser->PublicLog_Contents_SendAddGem(ePublicLogReason::REASON_ADD_GEM_BINGO_EVENT_REWARD, 0, AddGem, AddFreeGem);

					// 쿠폰 획득 로그
					for (const auto& coupon : repo_2)
					{
						auto couponBeginTime = coupon.m_tmBeginTime;
						auto couponEndTime = coupon.m_tmEndTime;
						GLOBAL::SendLog(userID, 0, DB_LOG::REASON_COUPON_GET, 0, 0, { coupon.m_nUniqueID, coupon.m_nItemKind, 0, 1, 0,
							static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)), static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)) }, {});
					}

					pResultUser->GetAssets().AddAssets(addAsset);

					std::unordered_map<INT32, INT64> mapDirectUse;
					std::vector<INT32> newItemKinds;
					for (const auto& item : vecAddItems)
					{
						INT64 prevNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						newItemKinds.push_back(item.Kind);

						auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
						if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
						{
							if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
								mapDirectUse[item.Kind] += item.Count;
							else
								mapDirectUse.emplace(item.Kind, item.Count);
						}

						INT64 resultNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, item.Kind, prevNum, item.Count, resultNum }, {});
					}

					for (const auto& coupon : vecCoupons)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, coupon.ItemKind, 0, 1, 1 }, {});
					}

					for (const auto& deleteItem : vecDeleteItems)
					{
						INT64 resultItemNum = pResultUser->GetInventory().GetItemNum(deleteItem.Kind);
						INT64 prevNum = resultItemNum + deleteItem.Count;
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_ITEM_USE, 0, 0,
							{ eventID, prevRound, afterRound, bingoNum_log, deleteItem.Kind, prevNum, deleteItem.Count, resultItemNum }, {});
					}

					std::vector<INT32> forLog;
					for (const auto& rewardType : rewardTypes) forLog.emplace_back(rewardType);
					for (int i = forLog.size(); i < 5; ++i)    forLog.emplace_back(-1);
					GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BINGO_EVENT_REWARD_TYPE, 0, 0,
						{ eventID, prevRound, afterRound, bingoNum_log, forLog[0], forLog[1], forLog[2], forLog[3], forLog[4] }, {});

					if (newItemKinds.size() > 0)
					{
						NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
						pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
						});
						SEND_DBA(pNEWITEMPACKET);
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
							for (const auto& item : vecAddItems)
								vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, pResultUser->GetInventory().GetItemNum(item.Kind)));

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
							for (const auto& coupon : repo_2)
							{
								auto couponBeginTime = coupon.m_tmBeginTime;
								auto couponEndTime = coupon.m_tmEndTime;
								vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
									::to_flatbuffer(fbb, coupon.m_szProductKinds)));
							}

							auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(), addAsset.GetGold());
							auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, keyItemKind, keyItemCount, bingo, bingoCount + 1, 0,0,afterRound,0, fbb.CreateVector(vecFlatRewardIDs), 0,
								sendAddAsset, curAsset, fbb.CreateVector(vecFlatAddItems), AddGem, AddFreeGem, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}

					if (mapDirectUse.size() > 0)
					{
						for (auto& itemDU : mapDirectUse)
						{
							auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
							ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
						}
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayBingo. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}


std::optional<std::tuple<INT32, INT32>> CustomEventHelper::GetRandomRouletteNumber(const INT64 eventID, const INT32 curRound, const INT32 prevScore, const GAME::eNATION nation, OUT std::set<std::tuple<INT32, INT32>>& rewardTypes)
{
	rewardTypes.clear();

	std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, -1, nation, OUT out_vecReward);

	std::vector<CustomEvent::ProbabilityInfo::SharedPtr> out_vecProbability;
	CustomEvent::CCustomEventInfoManager::Instance()->GetProbabilityInfos(eventID, OUT out_vecProbability);

	if (out_vecProbability.size() <= 0)
		return std::nullopt;

	// 확률 값 합계 합산
	INT32 totalProb = 0LL;
	for (const auto& iter_prob1 : out_vecProbability)
	{
		auto pProbInfo = (iter_prob1).get();
		if (pProbInfo == nullptr)
			continue;

		totalProb += pProbInfo->Probability;
	}

	// Rand!!
	INT32 ratio = CRandManager::RAND(EnumRandType::EVENT_BASE, 0, totalProb);

	// Roulette!!
	INT32 rouletteNumber = -1;
	INT32 addScore = -1;
	for (const auto& iter_prob2 : out_vecProbability)
	{
		auto pProbInfo = (iter_prob2).get();
		if (pProbInfo == nullptr)
			continue;

		if (ratio >= pProbInfo->Probability)
			ratio -= pProbInfo->Probability;
		else
		{
			rouletteNumber = static_cast<INT32>(pProbInfo->ProbKey_1);
			addScore = static_cast<INT32>(pProbInfo->ProbKey_2);
			break;
		}
	}

	if (rouletteNumber < 0 || addScore < 0)
		return std::nullopt;

	// 증가한 스코어로 얻게 될 보상 수집
	INT32 afterScore = prevScore + addScore;
	for (const auto& iter_reward : out_vecReward)
	{
		INT32 round = std::get<0>(iter_reward);
		auto pRewardInfo = (std::get<1>(iter_reward)).get();
		if (pRewardInfo == nullptr)
			continue;

		CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRoundInfo == nullptr)
			return std::nullopt;

		if (pRewardInfo->RewardType > 0)
		{
			if (pRoundInfo->RoundGoal.size() < pRewardInfo->RewardType)
				continue;

			INT32 rewardIdx = pRewardInfo->RewardType - 1;
			INT32 compareScore = pRoundInfo->RoundGoal[rewardIdx];
			if (prevScore < compareScore && compareScore <= afterScore)
			{
				if (false == rewardTypes.emplace(std::make_tuple(round, pRewardInfo->RewardType)).second)
					return std::nullopt;
			}
		}
		else
		{
			// 현재 라운드가 아니라면 최종 보상은 받을 수 없다
			if (curRound != round)
				continue;

			// 해당 라운드 최종 스코어에 도달했다면 라운드 보상 추가 지급
			INT32 finalScore = pRoundInfo->RoundGoal[pRoundInfo->RoundGoal.size() - 1];
			if (prevScore < finalScore && finalScore <= afterScore)
			{
				if (false == rewardTypes.emplace(std::make_tuple(round, 0)).second)
					return std::nullopt;
			}
		}
	}

	return std::make_optional<std::tuple<INT32, INT32>>(std::make_tuple(rouletteNumber, addScore));
}


bool CustomEventHelper::P_CUSTOM_EVENT_UPDATE_PlayRoulette(CUser* const pUser, const INT64 eventID, const INT32 curScore)
{
	// Play Roulette

	if (pUser == nullptr)
		return false;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	CRouletteEvent::SharedPtr spRouletteEvent = std::dynamic_pointer_cast<CRouletteEvent>(spEvent);
	if (spRouletteEvent == nullptr)
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < spEvent->m_StartTime || spEvent->m_EndTime < curTime)
		return false;

	if (spEvent->m_RewardKind <= 0)
		return false;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return false;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return false;

	// 따닥 방지
	if (spRouletteEvent->m_Score != curScore)
		return false;

	//if (pEventInfo->MaxRound <= spEvent->m_Round)
	{
		INT32 finalScore = pRoundInfo->RoundGoal[pRoundInfo->RoundGoal.size() - 1];
		if (finalScore <= spRouletteEvent->m_Score)
			return false;
	}

	INT32 keyItemKind = pEventInfo->KeyItemKind;
	INT64 keyItemCount = pEventInfo->KeyItemCount;

	BASE::ITEM_INFO* itemInfo = BASE::GET_ITEM_DATA(keyItemKind);
	if (itemInfo == nullptr || keyItemCount <= 0)
		return false;

	// 재료 아이템이 충분한지 검사
	if (false == pUser->GetInventory().IsItemEnough(keyItemKind, keyItemCount))
		return false;

	std::vector<BASE::REWARDITEM> vecDeleteItems;
	vecDeleteItems.emplace_back(BASE::REWARDITEM(keyItemKind, keyItemCount));

	// 이번 룰렛 랜덤 뽑기
	std::set<std::tuple<INT32, INT32>> rewardTypes;
	const auto rouletteResult = GetRandomRouletteNumber(eventID, spEvent->m_Round, spRouletteEvent->m_Score, pUser->GetNation(), OUT rewardTypes);
	if (false == rouletteResult.has_value())
		return false;

	INT32 rouletteNumber = std::get<0>(rouletteResult.value());
	INT32 addScore = std::get<1>(rouletteResult.value());

	// Valid Check
	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		// 현재 라운드거나 다음 라운드까지만 획득 가능
		if (round < spEvent->m_Round || spEvent->m_Round + 1 < round)
			return false;

		if (spEvent->IsRewarded(round, rewardType))
			return false;

		// 라운드 최종 보상의 경우 현재 라운드 보상이 아니라면 받을수 없다
		if (rewardType == 0 && round != spEvent->m_Round)
			return false;

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && spEvent->m_RewardKind != reward.Kind)
				continue;

			if (false == EventHelper::CheckValidItem(reward.Kind, curTime))
				return false;
		}
	}

	// 기존 값 backup
	INT32 prevProgress = spRouletteEvent->m_Score;
	INT32 prevRound = spEvent->m_Round;
	INT32 prevRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> prevRewardTypes;
	spEvent->GetReward(prevRound, OUT prevRewardTypes);

	////////// 인메모리 선처리 //////////
	INT32 afterProgress = spRouletteEvent->m_Score + addScore;
	spRouletteEvent->SetProgress(afterProgress);

	for (const auto& item : vecDeleteItems)
		pUser->GetInventory().AddItemNum(item.Kind, -item.Count);

	INT32 afterRound = -1;
	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		spEvent->SetReward(round, rewardType, 1);

		if (rewardType > 0)
			afterRound = std::max<INT32>(afterRound, round);
		else
			afterRound = std::max<INT32>(afterRound, round + 1);
	}

	if (-1 < afterRound && afterRound != spEvent->m_Round && prevRound < pEventInfo->MaxRound && afterRound <= pEventInfo->MaxRound)
	{
		spEvent->m_Round = afterRound;
		spEvent->m_RewardKind = 0;
	}
	else
	{
		afterRound = spEvent->m_Round;
	}
	//////////////////////////////////// 

	INT32 afterRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> afterRewardTypes;
	spEvent->GetReward(afterRound, OUT afterRewardTypes);

	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;

	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && prevRewardKind != reward.Kind)
				continue;

			auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
			if (iteminfo == nullptr)
				continue;

			if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
			{
				if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL)		    AddOil += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON)	    AddIron += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER)	AddSilver += reward.Count;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD)	    AddGold += reward.Count;
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE)	// 젬
			{
				if (iteminfo->i32VALUE[3] == 0) AddGem += iteminfo->i32VALUE[1] * reward.Count;	// 유가
				else							AddFreeGem += iteminfo->i32VALUE[1] * reward.Count;	// 무가
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_COUPON)		// 쿠폰
			{
				int64_t	nowTime = GetDueDay_UTC(0);
				int64_t	endTime = GetDueDay_UTC(iteminfo->i32VALUE[2]);

				if (GAME::eITEM_USE_TYPE::COUPON_INFINITY == iteminfo->i32ITEM_USE_TYPE)
				{
					endTime = GetDueDay_UTC(static_cast<INT64>(SECOND_PER_DAY) * 30LL * 12LL * 100LL);	// 100년 뒤
				}

				for (int iCnt = 0; iCnt < reward.Count; iCnt++)
				{
					auto couponData = BASE::COUPON_INFO_DATA.Get(iteminfo->i32ITEM_KIND);
					wstring strProductKinds = (nullptr == couponData) ? L"" : couponData->strPackageKind;
					vecCoupons.emplace_back(BASE::COUPON(0, 1, iteminfo->i32ITEM_KIND, nowTime, endTime, iteminfo->i32VALUE[1],
						strProductKinds));
				}
			}
			else	// 일반 아이템일때는 중복 체크하고 적재
			{
				bool isSet = false;
				for (int k = 0; k < vecAddItems.size(); ++k)
				{
					if (vecAddItems[k].Kind == reward.Kind)
					{
						vecAddItems[k].Count += reward.Count;
						isSet = true;
					}
				}
				if (false == isSet)
					vecAddItems.emplace_back(BASE::REWARDITEM(reward.Kind, reward.Count));
			}
		}
	}

	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = afterProgress;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();
			for (const auto& item : vecDeleteItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->TVP_3.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_3.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_3.m_nItemCount.push_back(item.Count);
				pQuery->TVP_3.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;

			pQuery->TVP_4.clear();
			for (const auto& coupon : vecCoupons)
			{
				pQuery->TVP_4.m_nItemKind.push_back(coupon.ItemKind);
				pQuery->TVP_4.m_nUniqueID.push_back(coupon.UniqueID);
				pQuery->TVP_4.m_nisActive.push_back(coupon.isActive);
				pQuery->TVP_4.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
				pQuery->TVP_4.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
				pQuery->TVP_4.m_nDiscountRate.push_back(coupon.DiscountRate);

				NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
				pQuery->TVP_4.m_nProductKinds.push_back(strProductKinds);
				pQuery->TVP_4.m_nProductKindsLen.push_back(static_cast<Int64>(strProductKinds.GetLength()) * static_cast<Int64>(strProductKinds.GetCharTypeSize()));
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
						{
							if (auto rollbackRoulette = std::dynamic_pointer_cast<CRouletteEvent>(rollbackEvent))
								rollbackRoulette->SetProgress(prevProgress);

							rollbackEvent->m_RewardMap.clear();
							for (const auto& rewardType : prevRewardTypes)
								rollbackEvent->SetReward(prevRound, rewardType, 0);

							rollbackEvent->m_Round = prevRound;
							rollbackEvent->m_RewardKind = prevRewardKind;
						}

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					// 자원 획득 후처리
					PROTOCOL::ASSET addAsset;
					if (AddOil > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetOil();
						addAsset.AddOil(AddOil);
						INT64 aftrNum = pResultUser->GetAssets().GetOil();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL, prevNum, AddOil, aftrNum }, {});
					}
					if (AddIron > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetIron();
						addAsset.AddIron(AddIron);
						INT64 aftrNum = pResultUser->GetAssets().GetIron();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON, prevNum, AddIron, aftrNum }, {});
					}
					if (AddSilver > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetSilver();
						addAsset.AddSilver(AddSilver);
						INT64 aftrNum = pResultUser->GetAssets().GetSilver();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER, prevNum, AddSilver, aftrNum }, {});
					}
					if (AddGold > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetGold();
						addAsset.AddGold(AddGold);
						INT64 aftrNum = pResultUser->GetAssets().GetGold();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD, prevNum, AddGold, aftrNum }, {});
					}

					if (AddGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::VC_GEM_KIND, 0, AddGem, 0 }, {});
					}

					if (AddFreeGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, GAME::FREE_GEM_KIND, 0, AddFreeGem, 0 }, {});
					}

					// 쿠폰 획득 로그
					for (const auto& coupon : repo_2)
					{
						auto couponBeginTime = coupon.m_tmBeginTime;
						auto couponEndTime = coupon.m_tmEndTime;
						GLOBAL::SendLog(userID, 0, DB_LOG::REASON_COUPON_GET, 0, 0, { coupon.m_nUniqueID, coupon.m_nItemKind, 0, 1, 0,
							static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)), static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)) }, {});
					}

					pResultUser->GetAssets().AddAssets(addAsset);

					std::unordered_map<INT32, INT32> mapDirectUse;
					std::vector<INT32> newItemKinds;
					for (const auto& item : vecAddItems)
					{
						INT64 prevNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						newItemKinds.push_back(item.Kind);

						auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
						if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
						{
							if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
								mapDirectUse[item.Kind] += item.Count;
							else
								mapDirectUse.emplace(item.Kind, item.Count);
						}

						INT64 resultNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, item.Kind, prevNum, item.Count, resultNum }, {});
					}

					for (const auto& coupon : vecCoupons)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, addScore, coupon.ItemKind, 0, 1, 1 }, {});
					}

					for (const auto& deleteItem : vecDeleteItems)
					{
						INT64 resultItemNum = pResultUser->GetInventory().GetItemNum(deleteItem.Kind);
						INT64 prevNum = resultItemNum + deleteItem.Count;
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_ITEM_USE, 0, 0,
							{ eventID, deleteItem.Kind, prevNum, deleteItem.Count, resultItemNum, prevRound, afterRound, addScore, prevProgress, afterProgress }, {});
					}

					for (const auto& tuple : rewardTypes)
					{
						INT32 round = std::get<0>(tuple);
						INT32 rewardType = std::get<1>(tuple);

						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_ROULETTE_EVENT_REWARD_TYPE, 0, 0,
							{ eventID, prevRound, afterRound, addScore, round, rewardType }, {});
					}

					if (newItemKinds.size() > 0)
					{
						NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
						pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
						});
						SEND_DBA(pNEWITEMPACKET);
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
							for (const auto& item : vecAddItems)
								vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, pResultUser->GetInventory().GetItemNum(item.Kind)));

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
							for (const auto& coupon : repo_2)
							{
								auto couponBeginTime = coupon.m_tmBeginTime;
								auto couponEndTime = coupon.m_tmEndTime;
								vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
									::to_flatbuffer(fbb, coupon.m_szProductKinds)));
							}

							auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(), addAsset.GetGold());
							auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, keyItemKind, keyItemCount, rouletteNumber, afterProgress, 0,0,afterRound,0, fbb.CreateVector(vecFlatRewardIDs), 0,
								sendAddAsset, curAsset, fbb.CreateVector(vecFlatAddItems), AddGem, AddFreeGem, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}

					if (mapDirectUse.size() > 0)
					{
						for (auto& itemDU : mapDirectUse)
						{
							auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
							ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
						}
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}


bool CustomEventHelper::P_CUSTOM_EVENT_UPDATE_PlayWarDice(CUser* const pUser, const INT64 eventID, const INT32 curScore)
{
	// Play WarDice

	if (pUser == nullptr)
		return false;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return false;

	CWarDiceEvent::SharedPtr spWarDiceEvent = std::dynamic_pointer_cast<CWarDiceEvent>(spEvent);
	if (spWarDiceEvent == nullptr)
		return false;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < spEvent->m_StartTime || spEvent->m_EndTime < curTime)
		return false;

	if (spEvent->m_RewardKind <= 0)
		return false;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return false;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return false;

	INT32 keyItemKind = pEventInfo->KeyItemKind;
	INT64 keyItemCount = pEventInfo->KeyItemCount;

	BASE::ITEM_INFO* itemInfo = BASE::GET_ITEM_DATA(keyItemKind);
	if (itemInfo == nullptr || keyItemCount <= 0)
		return false;

	// 재료 아이템이 충분한지 검사
	if (false == pUser->GetInventory().IsItemEnough(keyItemKind, keyItemCount))
		return false;

	std::vector<BASE::REWARDITEM> vecDeleteItems;
	vecDeleteItems.emplace_back(BASE::REWARDITEM(keyItemKind, keyItemCount));

	// 이번 룰렛 랜덤 뽑기
	std::set<std::tuple<INT32, INT32>> rewardTypes;
	const auto diceResult = GetDiceNumber(eventID, spEvent->m_Round, spWarDiceEvent->m_DiceBlock, pUser->GetNation(), OUT rewardTypes);
	if (diceResult <= 0)
		return false;

	// Valid Check
	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		// 현재 라운드거나 다음 라운드까지만 획득 가능
		if (round < spEvent->m_Round || spEvent->m_Round + 1 < round)
			return false;

		if (spEvent->IsRewarded(round, rewardType))
			return false;

		// 라운드 최종 보상의 경우 현재 라운드 보상이 아니라면 받을수 없다
		if (rewardType == 0 && round != spEvent->m_Round)
			return false;

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && spEvent->m_RewardKind != reward.Kind)
				continue;

			if (false == EventHelper::CheckValidItem(reward.Kind, curTime))
				return false;
		}
	}

	// 기존 값 backup
	INT32 prevBlock = spWarDiceEvent->m_DiceBlock;
	INT32 prevRound = spEvent->m_Round;
	INT32 prevRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> prevRewardTypes;
	spEvent->GetReward(prevRound, OUT prevRewardTypes);

	////////// 인메모리 선처리 //////////
	std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_curRoundReward;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, spEvent->m_Round, pUser->GetNation(), OUT out_curRoundReward);

	INT32 nextBlock = spWarDiceEvent->m_DiceBlock + diceResult;
	INT32 curRoundMaxCount = out_curRoundReward.size();

	if (nextBlock >= curRoundMaxCount)
		nextBlock -= curRoundMaxCount;

	spWarDiceEvent->SetProgress(nextBlock);

	for (const auto& item : vecDeleteItems)
		pUser->GetInventory().AddItemNum(item.Kind, -item.Count);

	INT32 afterRound = -1;
	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		spEvent->SetReward(round, rewardType, 1);

		if (rewardType > 0)
			afterRound = std::max<INT32>(afterRound, round);
		else
			afterRound = std::max<INT32>(afterRound, round + 1);
	}

	if (-1 < afterRound && afterRound != spEvent->m_Round && prevRound < pEventInfo->MaxRound && afterRound <= pEventInfo->MaxRound)
	{
		spEvent->m_Round = afterRound;
		spEvent->m_RewardKind = 0;
	}
	else
	{
		afterRound = spEvent->m_Round;
	}
	//////////////////////////////////// 

	INT32 afterRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> afterRewardTypes;
	spEvent->GetReward(afterRound, OUT afterRewardTypes);

	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;

	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		INT32 cityMagnification = CustomEvent::CCustomEventInfoManager::Instance()->GetWarDiceMagnification(eventID, round, nextBlock);

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && prevRewardKind != reward.Kind)
				continue;

			// 라운드 Complete 보상에는 cityMagnification를 적용해선 안된다
			INT32 bonusRate = (rewardType != 0) ? cityMagnification : 1;

			auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
			if (iteminfo == nullptr)
				continue;

			if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
			{
				if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL)		    AddOil += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON)	    AddIron += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER)	AddSilver += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD)	    AddGold += reward.Count * bonusRate;
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE)	// 젬
			{
				if (iteminfo->i32VALUE[3] == 0) AddGem += iteminfo->i32VALUE[1] * reward.Count * bonusRate;	// 유가
				else							AddFreeGem += iteminfo->i32VALUE[1] * reward.Count * bonusRate;	// 무가
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_COUPON)		// 쿠폰
			{
				int64_t	nowTime = GetDueDay_UTC(0);
				int64_t	endTime = GetDueDay_UTC(iteminfo->i32VALUE[2]);

				if (GAME::eITEM_USE_TYPE::COUPON_INFINITY == iteminfo->i32ITEM_USE_TYPE)
				{
					endTime = GetDueDay_UTC(static_cast<INT64>(SECOND_PER_DAY) * 30LL * 12LL * 100LL);	// 100년 뒤
				}

				for (int iCnt = 0; iCnt < reward.Count * bonusRate; iCnt++)
				{
					auto couponData = BASE::COUPON_INFO_DATA.Get(iteminfo->i32ITEM_KIND);
					wstring strProductKinds = (nullptr == couponData) ? L"" : couponData->strPackageKind;
					vecCoupons.emplace_back(BASE::COUPON(0, 1, iteminfo->i32ITEM_KIND, nowTime, endTime, iteminfo->i32VALUE[1],
						strProductKinds));
				}
			}
			else	// 일반 아이템일때는 중복 체크하고 적재
			{
				bool isSet = false;
				for (int k = 0; k < vecAddItems.size(); ++k)
				{
					if (vecAddItems[k].Kind == reward.Kind)
					{
						vecAddItems[k].Count += reward.Count;
						isSet = true;
					}
				}
				if (false == isSet)
					vecAddItems.emplace_back(BASE::REWARDITEM(reward.Kind, reward.Count * bonusRate));
			}
		}
	}


	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = nextBlock;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();
			for (const auto& item : vecDeleteItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->TVP_3.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_3.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_3.m_nItemCount.push_back(item.Count);
				pQuery->TVP_3.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;

			pQuery->TVP_4.clear();
			for (const auto& coupon : vecCoupons)
			{
				pQuery->TVP_4.m_nItemKind.push_back(coupon.ItemKind);
				pQuery->TVP_4.m_nUniqueID.push_back(coupon.UniqueID);
				pQuery->TVP_4.m_nisActive.push_back(coupon.isActive);
				pQuery->TVP_4.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
				pQuery->TVP_4.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
				pQuery->TVP_4.m_nDiscountRate.push_back(coupon.DiscountRate);

				NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
				pQuery->TVP_4.m_nProductKinds.push_back(strProductKinds);
				pQuery->TVP_4.m_nProductKindsLen.push_back(static_cast<Int64>(strProductKinds.GetLength()) * static_cast<Int64>(strProductKinds.GetCharTypeSize()));
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
						{
							if (auto rollbackRoulette = std::dynamic_pointer_cast<CRouletteEvent>(rollbackEvent))
								rollbackRoulette->SetProgress(prevBlock);

							rollbackEvent->m_RewardMap.clear();
							for (const auto& rewardType : prevRewardTypes)
								rollbackEvent->SetReward(prevRound, rewardType, 0);

							rollbackEvent->m_Round = prevRound;
							rollbackEvent->m_RewardKind = prevRewardKind;
						}

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					// 자원 획득 후처리
					PROTOCOL::ASSET addAsset;
					if (AddOil > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetOil();
						addAsset.AddOil(AddOil);
						INT64 aftrNum = pResultUser->GetAssets().GetOil();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL, prevNum, AddOil, aftrNum }, {});
					}
					if (AddIron > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetIron();
						addAsset.AddIron(AddIron);
						INT64 aftrNum = pResultUser->GetAssets().GetIron();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON, prevNum, AddIron, aftrNum }, {});
					}
					if (AddSilver > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetSilver();
						addAsset.AddSilver(AddSilver);
						INT64 aftrNum = pResultUser->GetAssets().GetSilver();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER, prevNum, AddSilver, aftrNum }, {});
					}
					if (AddGold > 0)
					{
						INT64 prevNum = pResultUser->GetAssets().GetGold();
						addAsset.AddGold(AddGold);
						INT64 aftrNum = pResultUser->GetAssets().GetGold();
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD, prevNum, AddGold, aftrNum }, {});
					}

					if (AddGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::VC_GEM_KIND, 0, AddGem, 0 }, {});
					}

					if (AddFreeGem > 0)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, GAME::FREE_GEM_KIND, 0, AddFreeGem, 0 }, {});
					}

					// 쿠폰 획득 로그
					for (const auto& coupon : repo_2)
					{
						auto couponBeginTime = coupon.m_tmBeginTime;
						auto couponEndTime = coupon.m_tmEndTime;
						GLOBAL::SendLog(userID, 0, DB_LOG::REASON_COUPON_GET, 0, 0, { coupon.m_nUniqueID, coupon.m_nItemKind, 0, 1, 0,
							static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)), static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)) }, {});
					}

					pResultUser->GetAssets().AddAssets(addAsset);

					std::unordered_map<INT32, INT32> mapDirectUse;
					std::vector<INT32> newItemKinds;
					for (const auto& item : vecAddItems)
					{
						INT64 prevNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						newItemKinds.push_back(item.Kind);

						auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
						if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
						{
							if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
								mapDirectUse[item.Kind] += item.Count;
							else
								mapDirectUse.emplace(item.Kind, item.Count);
						}

						INT64 resultNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, item.Kind, prevNum, item.Count, resultNum }, {});
					}

					for (const auto& coupon : vecCoupons)
					{
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_ITEM, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, coupon.ItemKind, 0, 1, 1 }, {});
					}

					for (const auto& deleteItem : vecDeleteItems)
					{
						INT64 resultItemNum = pResultUser->GetInventory().GetItemNum(deleteItem.Kind);
						INT64 prevNum = resultItemNum + deleteItem.Count;
						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_ITEM_USE, 0, 0,
							{ eventID, deleteItem.Kind, prevNum, deleteItem.Count, resultItemNum, nextBlock, prevRound, afterRound  }, {});
					}

					for (const auto& tuple : rewardTypes)
					{
						INT32 rewardType = std::get<1>(tuple);

						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_WARDICE_EVENT_REWARD_TYPE, 0, 0,
							{ eventID, prevRound, afterRound, nextBlock, rewardType }, {});
					}

					if (newItemKinds.size() > 0)
					{
						NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
						pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
						});
						SEND_DBA(pNEWITEMPACKET);
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
							for (const auto& item : vecAddItems)
								vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, pResultUser->GetInventory().GetItemNum(item.Kind)));

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
							for (const auto& coupon : repo_2)
							{
								auto couponBeginTime = coupon.m_tmBeginTime;
								auto couponEndTime = coupon.m_tmEndTime;
								vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
									::to_flatbuffer(fbb, coupon.m_szProductKinds)));
							}

							auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(), addAsset.GetGold());
							auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, keyItemKind, keyItemCount, diceResult, nextBlock, 0, 0, afterRound, 0, fbb.CreateVector(vecFlatRewardIDs), 0,
								sendAddAsset, curAsset, fbb.CreateVector(vecFlatAddItems), AddGem, AddFreeGem, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}

					if (mapDirectUse.size() > 0)
					{
						for (auto& itemDU : mapDirectUse)
						{
							auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
							ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
						}
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayWarDice. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}

INT32 CustomEventHelper::GetDiceNumber(const INT64 eventID, const INT32 curRound, const INT32 prevScore, const GAME::eNATION nation, OUT std::set<std::tuple<INT32, INT32>>& rewardTypes)
{
	rewardTypes.clear();

	std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, -1, nation, OUT out_vecReward);

	std::vector<CustomEvent::ProbabilityInfo::SharedPtr> out_vecProbability;
	CustomEvent::CCustomEventInfoManager::Instance()->GetProbabilityInfos(eventID, OUT out_vecProbability);

	if (out_vecProbability.size() <= 0)
		return 0;

	// 확률 값 합계 합산
	INT32 totalProb = 0LL;
	for (const auto& iter_prob1 : out_vecProbability)
	{
		auto pProbInfo = (iter_prob1).get();
		if (pProbInfo == nullptr)
			continue;

		totalProb += pProbInfo->Probability;
	}
	// Rand!!

	INT32 ratio = CRandManager::RAND(EnumRandType::EVENT_BASE, 0, totalProb);

	INT32 DiceReuslt = 0;
	for (const auto& iter_prob2 : out_vecProbability)
	{
		auto pProbInfo = (iter_prob2).get();
		if (pProbInfo == nullptr)
			continue;

		if (ratio >= pProbInfo->Probability)
			ratio -= pProbInfo->Probability;
		else
		{
			DiceReuslt = static_cast<INT32>(pProbInfo->ProbKey_1);
			break;
		}
	}

	if (DiceReuslt <= 0)
		return 0;

	//다음 발판 보상 수집
	INT32 afterBlock = prevScore + DiceReuslt;

	std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_curRoundReward;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, curRound, nation, OUT out_curRoundReward);

	INT32 curRoundMaxCount = out_curRoundReward.size();
	bool nextRound = false;
	INT32 compareRound = curRound;

	//만약에 시작발판을 지나갔으면, 초기화 해줘야함
	if (afterBlock >= curRoundMaxCount)
	{
		//여기까지 왔으면 다음 라운드 라는 소리.
		afterBlock -= curRoundMaxCount;
		nextRound = true;
		++compareRound;
	}

	for (const auto& iter_reward : out_vecReward)
	{
		INT32 round = std::get<0>(iter_reward);
		auto pRewardInfo = (std::get<1>(iter_reward)).get();
		if (pRewardInfo == nullptr)
			continue;

		CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRoundInfo == nullptr)
			return 0;

		if (pRewardInfo->RewardType > 0)
		{
			//워 다이스는 RoundGoal이 존재하지 않는다.
			if (pRewardInfo->RewardType != afterBlock)
				continue;

			if (compareRound == round)
			{
				if (rewardTypes.emplace(std::make_tuple(round, pRewardInfo->RewardType)).second == false)
					return 0;
			}
		}
		else
		{
			// 현재 라운드가 아니라면 최종 보상은 받을 수 없다
			if (curRound != round)
				continue;

			//다음 발판이 시작점이거나 지나갈 때
			if (nextRound == true)
			{
				if (false == rewardTypes.emplace(std::make_tuple(round, 0)).second)
					return 0;
			}
		}
	}


	return DiceReuslt;
}



void CustomEventHelper::GM_CUSTOM_EVNET_PROGRESS_SET(CUser* const pUser, const INT64 eventID, const INT32 point)
{
	auto spScheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (spScheduleInfo == nullptr)
		return;

	switch (spScheduleInfo->EventType)
	{
		case CustomEvent::eEventType::BINGO:
		{
			//
		}
		break;
		case CustomEvent::eEventType::ROULETTE:
		{
			CustomEventHelper::P_GM_CUSTOM_EVENT_UPDATE_PlayRoulette(pUser, eventID, point);
		}
		break;
	}
}

bool CustomEventHelper::P_GM_CUSTOM_EVENT_UPDATE_PlayRoulette(CUser* const pUser, const INT64 eventID, const INT32 point)
{
	auto spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr)
		return false;

	auto spScheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (spScheduleInfo == nullptr)
		return false;

	std::vector<CustomEvent::RoundInfo::SharedPtr> out_vecRound;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfos(eventID, OUT out_vecRound);
	INT32 iMaxRound = out_vecRound.size();

	if (auto spRouletteEvent = std::dynamic_pointer_cast<CRouletteEvent>(spEvent))
	{
		INT32 beforeRound     = spEvent->m_Round;
		INT32 afterRound      = -1;
		INT32 addScore        = point;
		INT32 roundCheck      = spEvent->m_Round;
		INT32 scoreCheck      = spRouletteEvent->m_Score;

		for (int i = point; i >= 0; )
		{
			CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, roundCheck);
			if (pRoundInfo == nullptr)
				break;

			int iRemainPoint = pRoundInfo->RoundGoal[pRoundInfo->RoundGoal.size() - 1] - scoreCheck;
			if (iRemainPoint > i)
			{
				addScore = i;
				afterRound = roundCheck;
				break;
			}
			else
			{
				i -= iRemainPoint;
				scoreCheck += iRemainPoint;
				++roundCheck;

				// 최종 라운드
				if (roundCheck >= iMaxRound)
				{
					addScore = i;
					afterRound = iMaxRound;
				}
			}
		}

		if (afterRound == -1)
			return false;

		// 기존 값 backup
		INT32 prevProgress   = spRouletteEvent->m_Score;
		INT32 prevRound      = spEvent->m_Round;
		INT32 prevRewardKind = spEvent->m_RewardKind;

		std::vector<INT32> prevRewardTypes;
		spEvent->GetReward(prevRound, OUT prevRewardTypes);

		////////// 인메모리 선처리 //////////
		INT32 afterProgress = spRouletteEvent->m_Score + point;
		spRouletteEvent->SetProgress(afterProgress);

		spEvent->m_Round = afterRound;

		if (beforeRound != afterRound)
			spEvent->m_RewardKind = 0;

		//////////////////////////////////// 
		CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, afterRound);
		if (pRoundInfo == nullptr)
			return false;

		std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
		CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, afterRound, pUser->GetNation(), OUT out_vecReward);
		for (const auto& iter_reward : out_vecReward)
		{
			//INT32 round = std::get<0>(iter_reward);
			auto pRewardInfo = (std::get<1>(iter_reward)).get();
			if (pRewardInfo == nullptr)
				continue;

			if (pRewardInfo->RewardType > 0)
			{
				INT32 rewardIdx = pRewardInfo->RewardType - 1;
				INT32 compareScore = pRoundInfo->RoundGoal[rewardIdx];
				if (compareScore <= afterProgress)
					spEvent->SetReward(afterRound, pRewardInfo->RewardType, 1);
			}
			else
			{
				INT32 finalScore = pRoundInfo->RoundGoal[pRoundInfo->RoundGoal.size() - 1];
				if (finalScore <= afterProgress)
					spEvent->SetReward(afterRound, pRewardInfo->RewardType, 1);
			}
		}


		std::vector<INT32> afterRewardTypes;
		spEvent->GetReward(afterRound, OUT afterRewardTypes);

		//INT32 nServerID = pUser->GetServerID();
		INT64 userID = pUser->UID();
		INT32 afterRewardKind = spEvent->m_RewardKind;
		INT32 keyItemKind = spScheduleInfo->KeyItemKind;

		::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = afterProgress;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();
			pQuery->TVP_3.clear();
			pQuery->TVP_4.clear();

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
						{
							if (auto rollbackRoulette = std::dynamic_pointer_cast<CRouletteEvent>(rollbackEvent))
								rollbackRoulette->SetProgress(prevProgress);

							rollbackEvent->m_RewardMap.clear();
							for (const auto& rewardType : prevRewardTypes)
								rollbackEvent->SetReward(prevRound, rewardType, 0);

							rollbackEvent->m_Round = prevRound;
							rollbackEvent->m_RewardKind = prevRewardKind;
						}

						return RECV_OK;
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
							for (const auto& coupon : repo_2)
							{
								auto couponBeginTime = coupon.m_tmBeginTime;
								auto couponEndTime = coupon.m_tmEndTime;
								vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
									::to_flatbuffer(fbb, coupon.m_szProductKinds)));
							}

							auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, 0, 0, 0, 0);
							auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, keyItemKind, 0, 0, afterProgress,0, 0,afterRound,0, fbb.CreateVector(vecFlatRewardIDs), 0,
								sendAddAsset, curAsset, fbb.CreateVector(vecFlatAddItems), 0, 0, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	}
	return true;
}

void CustomEventHelper::GM_WARDICE_SOLDIER_MOVE(CUser* const pUser, const INT64 eventID, const INT32 moveCount)
{
	if (pUser == nullptr)
		return;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return;

	CWarDiceEvent::SharedPtr spWarDiceEvent = std::dynamic_pointer_cast<CWarDiceEvent>(spEvent);
	if (spWarDiceEvent == nullptr)
		return;

	INT64 curTime = GetDueDay_UTC(0);
	if (curTime < spEvent->m_StartTime || spEvent->m_EndTime < curTime)
		return;

	if (spEvent->m_RewardKind <= 0)
		return;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return;

	std::set<std::tuple<INT32, INT32>> rewardTypes;
	rewardTypes.clear();

	std::vector<std::tuple<INT32, CustomEvent::RewardInfo::SharedPtr>> out_vecReward;
	CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfos(eventID, -1, pUser->GetNation(), OUT out_vecReward);

	//치트로 입력한 발판의 보상만 수집
	INT32 afterBlock = moveCount;

	for (const auto& iter_reward : out_vecReward)
	{
		INT32 round = std::get<0>(iter_reward);
		auto pRewardInfo = (std::get<1>(iter_reward)).get();
		if (pRewardInfo == nullptr)
			continue;

		CustomEvent::RoundInfo::SharedPtr pRoundInfo2 = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRoundInfo2 == nullptr)
			return;

		if (pRewardInfo->RewardType > 0)
		{
			//워 다이스는 RoundGoal이 존재하지 않는다.
			if (pRewardInfo->RewardType != afterBlock)
				continue;

			if (spEvent->m_Round == round)
			{
				if (rewardTypes.emplace(std::make_tuple(round, pRewardInfo->RewardType)).second == false)
					return;
			}
		}
	}

	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		// 현재 라운드거나 다음 라운드까지만 획득 가능
		if (round < spEvent->m_Round || spEvent->m_Round + 1 < round)
			return;

		if (spEvent->IsRewarded(round, rewardType))
			continue;

		// 라운드 최종 보상의 경우 현재 라운드 보상이 아니라면 받을수 없다
		if (rewardType == 0 && round != spEvent->m_Round)
			return;

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && spEvent->m_RewardKind != reward.Kind)
				continue;

			if (false == EventHelper::CheckValidItem(reward.Kind, curTime))
				return;
		}
	}

	// 기존 값 backup
	INT32 prevBlock = spWarDiceEvent->m_DiceBlock;
	INT32 prevRound = spEvent->m_Round;
	INT32 prevRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> prevRewardTypes;
	spEvent->GetReward(prevRound, OUT prevRewardTypes);

	////////// 인메모리 선처리 //////////

	INT32 nextBlock = moveCount;

	spWarDiceEvent->SetProgress(nextBlock);
	
	INT32 afterRound = -1;
	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		spEvent->SetReward(round, rewardType, 1);

		if (rewardType > 0)
			afterRound = std::max<INT32>(afterRound, round);
		else
			afterRound = std::max<INT32>(afterRound, round + 1);
	}

	if (-1 < afterRound && afterRound != spEvent->m_Round && prevRound < pEventInfo->MaxRound && afterRound <= pEventInfo->MaxRound)
	{
		spEvent->m_Round = afterRound;
		spEvent->m_RewardKind = 0;
	}
	else
	{
		afterRound = spEvent->m_Round;
	}
	//////////////////////////////////// 

	INT32 afterRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> afterRewardTypes;
	spEvent->GetReward(afterRound, OUT afterRewardTypes);

	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;

	for (const auto& tuple : rewardTypes)
	{
		INT32 round = std::get<0>(tuple);
		INT32 rewardType = std::get<1>(tuple);

		CustomEvent::RoundInfo::SharedPtr pRewardRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, round);
		if (pRewardRoundInfo == nullptr)
			continue;

		INT32 groupID = (rewardType == 0) ? pRewardRoundInfo->RoundRewardGroupID : pRewardRoundInfo->FixRewardGroupID;

		CustomEvent::RewardInfo::SharedPtr pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, pUser->GetNation(), rewardType);
		if (pRewardInfo == nullptr)
			pRewardInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRewardInfo(groupID, GAME::eNATION::eNATION_NONE, rewardType);
		if (pRewardInfo == nullptr)
			continue;

		INT32 cityMagnification = CustomEvent::CCustomEventInfoManager::Instance()->GetWarDiceMagnification(eventID, round, nextBlock);

		for (const auto& reward : pRewardInfo->RewardItems)
		{
			// 라운드 Complete 보상은 선택한 보상만 받는다
			if (rewardType == 0 && prevRewardKind != reward.Kind)
				continue;

			// 라운드 Complete 보상에는 cityMagnification를 적용해선 안된다
			INT32 bonusRate = (rewardType != 0) ? cityMagnification : 1;

			auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
			if (iteminfo == nullptr)
				continue;

			if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
			{
				if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::OIL)		    AddOil += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::IRON)	    AddIron += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::SILVER)	AddSilver += reward.Count * bonusRate;
				else if (reward.Kind == GAME::eITEM_NATIVE_RESOURCE_TYPE::GOLD)	    AddGold += reward.Count * bonusRate;
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE)	// 젬
			{
				if (iteminfo->i32VALUE[3] == 0) AddGem += iteminfo->i32VALUE[1] * reward.Count * bonusRate;	// 유가
				else							AddFreeGem += iteminfo->i32VALUE[1] * reward.Count * bonusRate;	// 무가
			}
			else if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_TYPE_COUPON)		// 쿠폰
			{
				int64_t	nowTime = GetDueDay_UTC(0);
				int64_t	endTime = GetDueDay_UTC(iteminfo->i32VALUE[2]);

				if (GAME::eITEM_USE_TYPE::COUPON_INFINITY == iteminfo->i32ITEM_USE_TYPE)
				{
					endTime = GetDueDay_UTC(static_cast<INT64>(SECOND_PER_DAY) * 30LL * 12LL * 100LL);	// 100년 뒤
				}

				for (int iCnt = 0; iCnt < reward.Count * bonusRate; iCnt++)
				{
					auto couponData = BASE::COUPON_INFO_DATA.Get(iteminfo->i32ITEM_KIND);
					wstring strProductKinds = (nullptr == couponData) ? L"" : couponData->strPackageKind;
					vecCoupons.emplace_back(BASE::COUPON(0, 1, iteminfo->i32ITEM_KIND, nowTime, endTime, iteminfo->i32VALUE[1],
						strProductKinds));
				}
			}
			else	// 일반 아이템일때는 중복 체크하고 적재
			{
				bool isSet = false;
				for (int k = 0; k < vecAddItems.size(); ++k)
				{
					if (vecAddItems[k].Kind == reward.Kind)
					{
						vecAddItems[k].Count += reward.Count;
						isSet = true;
					}
				}
				if (false == isSet)
					vecAddItems.emplace_back(BASE::REWARDITEM(reward.Kind, reward.Count * bonusRate));
			}
		}
	}


	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = nextBlock;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();
	
			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->TVP_3.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_3.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_3.m_nItemCount.push_back(item.Count);
				pQuery->TVP_3.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;

			pQuery->TVP_4.clear();
			for (const auto& coupon : vecCoupons)
			{
				pQuery->TVP_4.m_nItemKind.push_back(coupon.ItemKind);
				pQuery->TVP_4.m_nUniqueID.push_back(coupon.UniqueID);
				pQuery->TVP_4.m_nisActive.push_back(coupon.isActive);
				pQuery->TVP_4.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
				pQuery->TVP_4.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
				pQuery->TVP_4.m_nDiscountRate.push_back(coupon.DiscountRate);

				NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
				pQuery->TVP_4.m_nProductKinds.push_back(strProductKinds);
				pQuery->TVP_4.m_nProductKindsLen.push_back(static_cast<Int64>(strProductKinds.GetLength()) * static_cast<Int64>(strProductKinds.GetCharTypeSize()));
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. Fail Result:{}", SET_1[0].m_nResult);

						CCustumEventBase::SharedPtr rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
						if (rollbackEvent != nullptr)
						{
							if (auto rollbackRoulette = std::dynamic_pointer_cast<CRouletteEvent>(rollbackEvent))
								rollbackRoulette->SetProgress(prevBlock);

							rollbackEvent->m_RewardMap.clear();
							for (const auto& rewardType : prevRewardTypes)
								rollbackEvent->SetReward(prevRound, rewardType, 0);

							rollbackEvent->m_Round = prevRound;
							rollbackEvent->m_RewardKind = prevRewardKind;
						}


						return RECV_OK;
					}

					// 자원 획득 후처리
					PROTOCOL::ASSET addAsset;
					if (AddOil > 0)
					{
						addAsset.AddOil(AddOil);
					}
					if (AddIron > 0)
					{
						addAsset.AddIron(AddIron);
					}
					if (AddSilver > 0)
					{
						addAsset.AddSilver(AddSilver);
					}
					if (AddGold > 0)
					{
						addAsset.AddGold(AddGold);
					}


					pResultUser->GetAssets().AddAssets(addAsset);

					std::unordered_map<INT32, INT32> mapDirectUse;
					std::vector<INT32> newItemKinds;
					for (const auto& item : vecAddItems)
					{
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						newItemKinds.push_back(item.Kind);

						auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
						if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
						{
							if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
								mapDirectUse[item.Kind] += item.Count;
							else
								mapDirectUse.emplace(item.Kind, item.Count);
						}

					}


					if (newItemKinds.size() > 0)
					{
						NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
						pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
						});
						SEND_DBA(pNEWITEMPACKET);
					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
							for (const auto& item : vecAddItems)
								vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, pResultUser->GetInventory().GetItemNum(item.Kind)));

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
							for (const auto& coupon : repo_2)
							{
								auto couponBeginTime = coupon.m_tmBeginTime;
								auto couponEndTime = coupon.m_tmEndTime;
								vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nIsActive, coupon.m_nItemKind,
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
									static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDIscountRate,
									::to_flatbuffer(fbb, coupon.m_szProductKinds)));
							}

							auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(), addAsset.GetGold());
							auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, 0, 0, moveCount, nextBlock, 0, 0, afterRound, 0, fbb.CreateVector(vecFlatRewardIDs), 0,
								sendAddAsset, curAsset, fbb.CreateVector(vecFlatAddItems), AddGem, AddFreeGem, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}

					if (mapDirectUse.size() > 0)
					{
						for (auto& itemDU : mapDirectUse)
						{
							auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
							ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
						}
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayWarDice. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return;

}

void CustomEventHelper::GM_CUSTOMEVENT_ROUND_SET(CUser* const pUser, const INT64 eventID, const INT32 round)
{
	if (pUser == nullptr)
		return;

	CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State != CustomEvent::eEventState::PROCESSING)
		return;

	CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (pEventInfo == nullptr)
		return;

	CustomEvent::RoundInfo::SharedPtr pRoundInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetRoundInfo(eventID, spEvent->m_Round);
	if (pRoundInfo == nullptr)
		return;

	INT32 afterRound = round;

	spEvent->m_Round = afterRound;
	spEvent->m_RewardKind = 0;
	spEvent->ClearProgress();

	//INT32 nServerID = pUser->GetServerID();
	INT64 userID = pUser->UID();
	INT32 afterRewardKind = spEvent->m_RewardKind;

	std::vector<INT32> afterRewardTypes;
	afterRewardTypes.clear();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_UPDATE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nRound = afterRound;
			pQuery->m_nProgress = 0;
			pQuery->m_nRewardKind = afterRewardKind;

			pQuery->TVP_1.clear();
			for (const auto& rewardType : afterRewardTypes)
			{
				pQuery->TVP_1.m_nKey.push_back(rewardType);
				pQuery->TVP_1.m_nValue.push_back(1);
			}

			// SelectRewardKind 와는 달리 여기서는 T_CustomEvent_Reward를 항상 초기화해줘야 한다
			if (afterRewardTypes.size() <= 0)
			{
				pQuery->TVP_1.m_nKey.push_back(1);
				pQuery->TVP_1.m_nValue.push_back(0);
			}

			pQuery->TVP_2.clear();

			pQuery->m_nAddOil = 0;
			pQuery->m_nAddIron = 0;
			pQuery->m_nAddSilver = 0;
			pQuery->m_nAddGold = 0;

			pQuery->TVP_3.clear();

			pQuery->m_nAddGem = 0;
			pQuery->m_nAddFreeGem = 0;

			pQuery->TVP_4.clear();

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayRoulette. Fail Result:{}", SET_1[0].m_nResult);

						return RECV_OK;
					}
				
					
					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_PLAY_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto // FlatBuffers 의 offset 을 리턴한다.
						{
							std::vector<INT32> vecFlatRewardIDs;
							for (const auto& rewardID : afterRewardTypes)
								vecFlatRewardIDs.emplace_back(rewardID);

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;

							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;


							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_PLAY_ACK(fbb, eventID, 0, 0, 0, 0, 0, 0, afterRound, 0, fbb.CreateVector(vecFlatRewardIDs), 0,
								0, 0, fbb.CreateVector(vecFlatAddItems), 0, 0, fbb.CreateVector(vecFlatCoupons));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);

						NEW_FLATBUFFER(GS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK, pPACKET_MAIN_REWARD);
						pPACKET_MAIN_REWARD.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_MAIN_REWARD_CHANGE_ACK(fbb, eventID, 0);
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET_MAIN_REWARD);

					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_UPDATE_PlayWarDice. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return ;
}

bool CustomEventHelper::P_CUSTOM_EVENT_COMPLETE(CUser* const pUser, const INT64 eventID)
{
	if (pUser == nullptr)
		return false;

	auto spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State == CustomEvent::eEventState::EXPIRED)
		return false;

	auto spScheduleInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (spScheduleInfo == nullptr)
		return false;

	INT64 userID = pUser->UID();
	//INT32 keyItemKind = spScheduleInfo->KeyItemKind;
	CustomEvent::eEventState prevState = spEvent->m_State;
	//INT64 keyItemCount = pUser->GetInventory().GetItemNum(keyItemKind);

	spEvent->m_State = CustomEvent::eEventState::EXPIRED;
	//pUser->GetInventory().AddItemNum(keyItemKind, -keyItemCount);

	std::vector<BASE::REWARDITEM> vecDeleteItems;
	//vecDeleteItems.emplace_back(BASE::REWARDITEM(keyItemKind, keyItemCount));

	std::vector<BASE::REWARDITEM> vecAddItems;

	//INT32 nServerID = pUser->GetServerID();
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_COMPLETE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nState = static_cast<UINT8>(CustomEvent::eEventState::EXPIRED);

			pQuery->TVP_1.clear();
			for (const auto& item : vecDeleteItems)
			{
				pQuery->TVP_1.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_1.m_nItemCount.push_back(item.Count);
				pQuery->TVP_1.m_nItemCount_Svr.push_back(0);
			}

			pQuery->TVP_2.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0)
					{
						if (auto spEvent2 = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID))
							spEvent2->m_State = prevState;

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					int resultValue = SET_1[0].m_nResult;
					if (-30 == resultValue)
					{
						LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. Fail Result:{}", resultValue);
						// -30 에러는 update 하려는 State 값이 이미 있다는 의미이다. m_State은 롤백할 필요 없음.
						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
					}
					else if (IS_FAILED(resultValue))
					{
						LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. Fail Result:{}", resultValue);

						if (auto spEvent2 = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID))
							spEvent2->m_State = prevState;

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					for (const auto& item : vecAddItems)
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}


bool CustomEventHelper::P_CUSTOM_EVENT_ITEMDELETE(CUser* const pUser, const INT64 eventID)
{
	if (pUser == nullptr)
		return false;

	auto spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
	if (spEvent == nullptr || spEvent->m_State == CustomEvent::eEventState::ITEMDELETE)
		return false;

	auto deleteItemInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetDeleteItemInfo(eventID);
	if (deleteItemInfo == nullptr)
		return false;

	auto mainEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
	if (mainEventInfo == nullptr)
		return false;

	INT64 userID = pUser->UID();
	CustomEvent::eEventState prevState = spEvent->m_State;

	spEvent->m_State = CustomEvent::eEventState::ITEMDELETE;

	std::vector<BASE::REWARDITEM> vecDeleteItems;

	for (int i = 0; i < deleteItemInfo->KeyItemKinds.size(); ++i)
	{
		int itemCount = pUser->GetItemCount(deleteItemInfo->KeyItemKinds[i]);
		if(itemCount > 0)
			vecDeleteItems.emplace_back(BASE::REWARDITEM(deleteItemInfo->KeyItemKinds[i], itemCount));
	}

	std::vector<BASE::REWARDITEM> vecAddItems;

	//INT32 nServerID = pUser->GetServerID();
	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOM_EVENT_COMPLETE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nEventID = eventID;
			pQuery->m_nState = static_cast<UINT8>(CustomEvent::eEventState::ITEMDELETE);

			pQuery->TVP_1.clear();
			for (const auto& item : vecDeleteItems)
			{
				pQuery->TVP_1.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_1.m_nItemCount.push_back(item.Count);
				pQuery->TVP_1.m_nItemCount_Svr.push_back(0);
			}

			pQuery->TVP_2.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0)
					{
						if (auto spEvent2 = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID))
							spEvent2->m_State = prevState;

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					int resultValue = SET_1[0].m_nResult;
					if (-30 == resultValue)
					{
						LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. Fail Result:{}", resultValue);
						// -30 에러는 update 하려는 State 값이 이미 있다는 의미이다. m_State은 롤백할 필요 없음.
						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
					}
					else if (IS_FAILED(resultValue))
					{
						LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. Fail Result:{}", resultValue);

						if (auto spEvent2 = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID))
							spEvent2->m_State = prevState;

						for (const auto& item : vecDeleteItems)
							pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						return RECV_OK;
					}

					for (const auto& item : vecAddItems)
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
										
					std::vector<PROTOCOL::ITEM_INFO> vecRewardItem;
					int DeleteItemCount = 0;
					STRING guildNickName;

					auto userGuild = CGuildManager::Instance()->GetGuild(pResultUser->GetGuildID());
					guildNickName = userGuild != nullptr ? userGuild->GetGuildNickName() : L"";

					for (int i = 0; i < deleteItemInfo->KeyItemKinds.size(); ++i)
						DeleteItemCount += pResultUser->GetItemCount(deleteItemInfo->KeyItemKinds[i]);

					for (const auto& item : vecDeleteItems)
					{
						pResultUser->GetInventory().SetItemNum(item.Kind, 0);
						GLOBAL::SendLogItemUse(userID, DB_LOG::REASON_EVENT_ITEM_DELETE_ITEM_USE, item.Kind, item.Count, {}, {});

						NEW_FLATBUFFER(GS_ITEM_SYNC_NFY, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::ITEM>> itemList;
							itemList.emplace_back(PROTOCOL::FLATBUFFERS::CreateITEM(fbb, item.Kind, 0, 0, 0, 0, 0));
							return PROTOCOL::FLATBUFFERS::CreateGS_ITEM_SYNC_NFY(fbb, fbb.CreateVector(itemList));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}

					if (DeleteItemCount > 0 && deleteItemInfo->RewardItems.size() > 0)
					{
						for (int i = 0; i < deleteItemInfo->RewardItems.size(); ++i)
							vecRewardItem.emplace_back(PROTOCOL::ITEM_INFO(deleteItemInfo->RewardItems[i].Kind, deleteItemInfo->RewardItems[i].Count * DeleteItemCount));

						std::vector<JSON::JsonMailParam> contentparams;
						contentparams.emplace_back(JSON::JsonMailParam(JSON::MailParamType::PARAMTYPE_TEXTKEY, mainEventInfo->TitleKey));

						CMailManager::Instance()->SendAppendingMail(
							pResultUser->UID(),
							guildNickName,
							pResultUser->GetLordName(),
							deleteItemInfo->TitleKey,
							deleteItemInfo->ContentsKey,
							vecRewardItem,
							GAME::eMAIL_TYPE::MAIL_TYPE_DELETE_EVENTITEM_REWARD,
							nullptr,
							&contentparams,
							L"",
							L"",
							L"",
							-1,
							GAME::eMAIL_CATEGORY::SYSTEM);
					}

				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOM_EVENT_COMPLETE. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});
	return true;
}

bool CustomEventHelper::P_CUSTOMEEVENT_QUEST_SET_TV(CUser* const pUser, std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecInfos, const bool sendClient)
{
	if (pUser == nullptr)
		return false;

	std::vector<std::tuple<INT64, INT64, INT64, CustomEvent::eCustomEventMissionState>> vecRollback;
	std::vector < std::tuple<INT64, INT64, std::unordered_map<INT32, BASE::REWARDITEM>>> vecLogRewardInfos;
	std::unordered_map<INT32, BASE::REWARDITEM> rewardItems;

	for (const auto& [EventID, QuestID, Value, State] : vecInfos)
	{
		INT64 eventID = EventID;
		INT64 questID = QuestID;
		INT64 value = Value;
		CustomEvent::eCustomEventMissionState state = State;

		CCustumEventBase::SharedPtr spEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(eventID);
		if (spEvent == nullptr)
			return false;

		if (spEvent->m_State != CustomEvent::eEventState::PROCESSING)
			return false;

		CustomEvent::ScheduleInfo::SharedPtr pEventInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetScheduleInfo(eventID);
		if (pEventInfo == nullptr)
			return false;

		if (spEvent->m_QuestMap.find(questID) == spEvent->m_QuestMap.end())
			return false;

		CCustomEventQuest::SharedPtr spQuest = spEvent->m_QuestMap.at(questID);
		if (spQuest == nullptr)
			return false;

		INT64 prevValue = spQuest->m_CurValue;
		CustomEvent::eCustomEventMissionState prevState = spQuest->m_MissionState;

		//선처리
		spQuest->m_CurValue = value;
		spQuest->m_MissionState = state;

		vecRollback.emplace_back(std::make_tuple(eventID, questID, prevValue, prevState));

		// 아이템은 후처리
		if (prevState == CustomEvent::eCustomEventMissionState::COMPLETE
		 && state == CustomEvent::eCustomEventMissionState::REWARDED)
		{
			auto questInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfo(spQuest->m_EventID, spQuest->m_QuestID);
			if (questInfo != nullptr)
			{
				std::unordered_map<INT32, BASE::REWARDITEM> logItmes;

				// 아이템
				for (const auto& item : questInfo->RewardItems)
				{
					const auto& find_iter = rewardItems.find(item.Kind);
					if (find_iter == rewardItems.end())
						rewardItems.emplace(item.Kind, BASE::REWARDITEM(item.Kind, item.Count));
					else
						find_iter->second.Count += item.Count;
				}

				for (const auto& item : questInfo->RewardItems)
				{
					const auto& find_iter = logItmes.find(item.Kind);
					if (find_iter == logItmes.end())
						logItmes.emplace(item.Kind, BASE::REWARDITEM(item.Kind, item.Count));
					else
						find_iter->second.Count += item.Count;
				}

				vecLogRewardInfos.emplace_back(std::make_tuple(EventID, QuestID, logItmes));
			}
		}
	}

	INT64 userID = pUser->UID();
	//INT32 serverID = pUser->GetServerID();

	::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_CUSTOMEVENT_QUEST_SET_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;

			pQuery->TVP_1.clear();
			pQuery->TVP_2.clear();
			for (const auto& [EventID, QuestID, Value, State] : vecInfos)
			{
				pQuery->TVP_1.m_nEventID.push_back(EventID);
				pQuery->TVP_1.m_nQuestID.push_back(QuestID);
				pQuery->TVP_1.m_nCurValue.push_back(Value);
				pQuery->TVP_1.m_nState.push_back(static_cast<UINT8>(State));
			}

			for (const auto& pair : rewardItems)
			{
				const auto& item = pair.second;
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (SET_1.size() <= 0 || IS_FAILED(SET_1[0].m_nResult))
					{
						if (SET_1.size() > 0)
							LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CHANGEEVENT_QUEST_SET_TV. Fail Result:{}", SET_1[0].m_nResult);

						for (const auto& tuple : vecRollback)
						{
							INT64 eventID = std::get<0>(tuple);
							INT64 questID = std::get<1>(tuple);
							INT64 prevValue = std::get<2>(tuple);
							CustomEvent::eCustomEventMissionState prevState = std::get<3>(tuple);

							auto rollbackEvent = ASE_INSTANCE(pResultUser, CCustomEventContainer)->Seek(eventID);
							if (rollbackEvent != nullptr)
							{
								if (rollbackEvent->m_QuestMap.end() != rollbackEvent->m_QuestMap.find(questID))
								{
									auto& rollbackQuest = rollbackEvent->m_QuestMap.at(questID);
									if (rollbackQuest != nullptr)
									{
										rollbackQuest->m_CurValue = prevValue;
										rollbackQuest->m_MissionState = prevState;
									}
								}
							}
						}

						return RECV_OK;
					}

					//로그용..!
					for (const auto& [eventID, questID, rewards] : vecLogRewardInfos)
					{
						for (const auto& pair : rewards)
						{
							const auto& item = pair.second;
							auto prevCount = pResultUser->GetInventory().GetItemNum(item.Kind);

							//EventID, QuestID, 0, 0 , 획득Kind, 이전수량, 이후 수량
							GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_TIMING_EVENT_MISSION_CLEAR, 0, 0,
								{ eventID, questID, 0, 0,item.Kind, prevCount, (prevCount + item.Count)}, {});
						}
					}

					std::unordered_map<INT32, INT64> mapDirectUse;
					std::vector<INT32> newItemKinds;
					for (const auto& pair : rewardItems)
					{
						const auto& item = pair.second;
						//INT64 prevNum = pResultUser->GetInventory().GetItemNum(item.Kind);
						pResultUser->GetInventory().AddItemNum(item.Kind, item.Count);
						newItemKinds.push_back(item.Kind);

						auto pItemInfo = BASE::GET_ITEM_DATA(item.Kind);
						if (pItemInfo != nullptr && pItemInfo->isDirectUse == true)
						{
							if (mapDirectUse.end() != mapDirectUse.find(item.Kind))
								mapDirectUse[item.Kind] += item.Count;
							else
								mapDirectUse.emplace(item.Kind, item.Count);
						}
					}

					
					

					if (sendClient == true)
					{
						NEW_FLATBUFFER(GS_CUSTOM_EVENT_QUEST_POINT_NFY, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::CUSTOM_EVENT_QUEST>> vecMissionInfos;

							for (const auto& [EventID, QuestID, Value, State] : vecInfos)
							{
								CCustumEventBase::SharedPtr pEvent = ASE_INSTANCE(pUser, CCustomEventContainer)->Seek(EventID);
								if (pEvent == nullptr)
									continue;

								CustomEvent::QuestInfo::SharedPtr pQuestInfo = CustomEvent::CCustomEventInfoManager::Instance()->GetQuestInfo(EventID, QuestID);
								if (pQuestInfo == nullptr)
									continue;

								auto iter = pEvent->m_QuestMap.find(QuestID);
								if (iter != pEvent->m_QuestMap.end())
								{
									std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatRewardItems;
									for (const auto& item : rewardItems)
										vecFlatRewardItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.second.Kind, item.second.Count));

									vecMissionInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateCUSTOM_EVENT_QUEST(
										fbb,
										EventID,
										QuestID,
										pQuestInfo->ConditionKind,
										pQuestInfo->ConditionValue,
										Value,
										pQuestInfo->TargetValue,
										CREATE_FLATBUFFER_RAWSTR(pQuestInfo->PointHelpKey.c_str()),
										static_cast<INT16>(State),
										fbb.CreateVector(vecFlatRewardItems)
									));
								}
							}

							return PROTOCOL::FLATBUFFERS::CreateGS_CUSTOM_EVENT_QUEST_POINT_NFY(fbb, fbb.CreateVector(vecMissionInfos));
						});
						SEND_ACTIVE_USER(pUser, pPACKET);

						if (newItemKinds.size() > 0)
						{
							NEW_FLATBUFFER(DB_NEWITEM_SET_REQ, pNEWITEMPACKET);
							pNEWITEMPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
							{
								return PROTOCOL::FLATBUFFERS::CreateDB_NEWITEM_SET_REQ(fbb, pResultUser->UID(), GLOBAL::GS_INFO.SVID, fbb.CreateVector(newItemKinds));
							});
							SEND_DBA(pNEWITEMPACKET);
						}

						if (mapDirectUse.size() > 0)
						{
							for (auto& itemDU : mapDirectUse)
							{
								auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
								ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
							}
						}
					}
				}
				else
				{
					LOGGER_DEBUG(CONST_EVENT_LOG, L"P_CUSTOMEVENT_QUEST_SET_TV. not found user. userid:{}", userID);
				}
				return RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});

	return true;
}

void CTimingGameEvent::ClearProgress()
{
	m_SuccessBarX = 0;
	m_GreatSuccessBarX = 0;
	m_Round = 0;
	m_BounsPoint = 0;
}

bool CTimingGameEvent::SetProgress(const INT32 successBar, const INT32 greatSuccessBar, const INT32 round, const INT32 bounsPoint)
{
	m_SuccessBarX = successBar;
	m_GreatSuccessBarX = greatSuccessBar;
	m_Round = round;
	m_BounsPoint = bounsPoint;
	return true;
}

void CWarDiceEvent::ClearProgress()
{
	m_DiceBlock = 0;
}

bool CWarDiceEvent::SetProgress(const INT32 block)
{
	m_DiceBlock = block;

	return true;
}


