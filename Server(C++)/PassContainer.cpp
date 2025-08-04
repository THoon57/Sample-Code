#include "GameAfx.h"

INT32 CBattlePass::GetPassDay() const
{
	const INT64 i64CurTime = GetDueDay_UTC(0);
	const INT64 i64Time = i64CurTime - m_StartTime;
	
	return static_cast<INT32>(ceil(static_cast<double>(i64Time) / static_cast<double>(GAME::ONE_DAY_TIME)));
}

INT32 CBattlePass::GetPassWeek() const
{
	const INT64 i64CurTime = GetDueDay_UTC(0);
	const INT64 iSeasonDuration = m_EndTime - m_StartTime;
	const double dValue = static_cast<double>(iSeasonDuration) / static_cast<double>(GAME::ONE_WEEK_TIME);
	const INT32 iMaxWeek = REVISE_FFZ(dValue);
	
	for (INT32 i = 1; i <= iMaxWeek; ++i)
	{
		if (i64CurTime < m_StartTime + (static_cast<INT64>(GAME::ONE_WEEK_TIME) * i))
		{
			return i;
		}
	}
	
	return 1;
}

// 몇 주차 미션까지 등록 완료했는지 확인하기 위한 용도 ( P_BATTLEPASS_LOAD 후 단 한번만 )
void CBattlePassContainer::SetUnlockWeekMission()
{
	for (const auto& [passKind, passSchedule] : repo_BattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;

		// 초기화 후 현재 등록된 미션들 기준으로 다시 세팅
		pUserPass->m_setCheckWeek.clear();

		auto& curMissionMap = pUserPass->m_MissionMap;
		for (const auto& [missionKind, passMission] : curMissionMap)
		{
			const auto missionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(passKind, missionKind);
			if (missionInfo->MissionScheduleType != BattlePass::ePassMissionType::WEEK)
				continue;

			pUserPass->m_setCheckWeek.emplace(missionInfo->MissionScheduleValue);
		}
	}
}

CBattlePassContainer::BattlePassDatum::SharedPtr CBattlePassContainer::SeekDatum(INT32 nKey)
{
	auto iterF = repo_BattlePass.find(nKey);
	if (iterF == repo_BattlePass.end())
		return nullptr;

	return (*iterF).second;
}

CBattlePass::SharedPtr CBattlePassContainer::Insert(CBattlePass::SharedPtr newData)
{
	if (newData == nullptr)
		return nullptr;

	auto newDatum = std::make_shared<BattlePassDatum>();
	if (newDatum == nullptr)
		throw std::bad_alloc();

	if (nullptr != Seek(newData->m_PassKind))
		Delete(newData->m_PassKind);

	newDatum->pBattlePass = newData;
	auto [iter_1, result_1] = repo_BattlePass.emplace(newData->m_PassKind, newDatum);
	if (false == result_1)
		return nullptr;

	newDatum->pBattlePass = newData;
	newDatum->iter_BattlePass = iter_1;
	
	return newData;
}

void CBattlePassContainer::Delete(INT64 nKey)
{
	auto pDatum = SeekDatum(nKey);
	if (pDatum == nullptr)
		return;

	repo_BattlePass.erase(pDatum->iter_BattlePass);
}


bool CBattlePassContainer::UpdateQuestCacheMap()
{
	repo_Mission.clear();

	CBattlePassMission::UpdateMissionInfos needUpdateMissions;
	CBattlePassMission::UpdateMissionInfos rollbackMissions;
	
	for (const auto& pair1 : repo_BattlePass)
	{
		auto& spBattlePass = (pair1).second->pBattlePass;
		if (spBattlePass == nullptr || spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
			continue;

		for (const auto& pair2 : spBattlePass->m_MissionMap)
		{
			auto& spUserMission = (pair2).second;
			if (spUserMission == nullptr)
				continue;

			if(spUserMission->m_MissionState != BattlePass::ePassMissionState::PROCESSING)
				continue;

			BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(spBattlePass->m_PassKind, spUserMission->m_MissionKind);
			if (spMissionInfo == nullptr)
				continue;

			repo_Mission.emplace(spMissionInfo->ConditionKind, spUserMission);

			// 현재 값
			const INT64 prevValue = spUserMission->m_CurValue;
			const BattlePass::ePassMissionState prevState = spUserMission->m_MissionState;

			INT64 afterValue = prevValue;
			BattlePass::ePassMissionState afterState = prevState;

			// 스칼라값
			const INT64 scalarValue = GLOBAL::GetMissionScalar(m_UserID, spMissionInfo->ConditionKind, spMissionInfo->ConditionValue, spUserMission->m_CurValue);

			// CurValue 및 State 값 보정
			if (afterValue < scalarValue)
				afterValue = scalarValue;
			if (spMissionInfo->TargetValue <= afterValue)
				afterState = BattlePass::ePassMissionState::COMPLETE;

			if (prevValue != afterValue || prevState != afterState)
			{
				needUpdateMissions.emplace_back(std::make_tuple(spBattlePass->m_PassKind, spBattlePass->m_Round, spUserMission->m_MissionKind, afterValue, afterState, spUserMission->m_RegistTime));
				rollbackMissions.emplace_back(std::make_tuple(spBattlePass->m_PassKind, spBattlePass->m_Round, spUserMission->m_MissionKind, prevValue, prevState, spUserMission->m_RegistTime));
			}

		}
	}

	// 컨테이너 empty가 아니면 미션 업데이트 프로시저 호출하기
	if (false == needUpdateMissions.empty())
	{
		if (CUser* pUser = CUserManager::Instance()->FindByUID(m_UserID))
			CBattlePassHelper::P_BATTLEPASS_MISSION_UPSERT_TV(pUser, needUpdateMissions);
	}
	
	return true;
}

void CBattlePassContainer::UpdateEvent(INT32 conditionKind, INT64 conditionValue, INT64 value, CQuestManager::SETTYPE setType, OUT CBattlePassMission::UpdateMissionInfos& vecNeedUpdate)
{
	if (GLOBAL::IsBattleRoyalServer() || GLOBAL::IsServerWarsServer() || CServerInvasionManager::Instance()->IsInvasionUser(m_UserID))
		return;

	auto& curMissionMap = GetMissionRepository();
	auto range = curMissionMap.equal_range(conditionKind);
	
	for (const auto& [condKey, wpMission] : GAME::for_range(curMissionMap.equal_range(conditionKind)))
	{
		CBattlePassMission::SharedPtr spMission = wpMission.lock();
		if (spMission == nullptr)
			continue;

		const CBattlePass::SharedPtr spBattlePass = Seek(spMission->m_PassKind);
		if (spBattlePass == nullptr)
			continue;

		const auto passScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(spBattlePass->m_PassKind);
		if (passScheduleInfo == nullptr)
			continue;
		
		if(spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
			continue;

		BattlePass::BattlePassMissionInfo::SharedPtr pMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(spMission->m_PassKind, spMission->m_MissionKind);
		if (pMissionInfo == nullptr)
			continue;
		
		if (pMissionInfo->ConditionKind != conditionKind)
			continue;

		// OnQuestEvent 로직은 너무 많이 호출되기 때문에 매번 캐싱 데이터 갱신하면 부하가 심히다.
		// 이 조건으로만 처리하고 유저가 미션 완료 요청 패킷을 보내면 그 때 캐싱 변경해주자.
		if (spMission->m_MissionState != BattlePass::ePassMissionState::PROCESSING)
			continue;

		INT64 prevValue = spMission->m_CurValue;
		INT64 afterValue = spMission->m_CurValue;

		if (BASE::QUEST_CONDITION_INFO::CanUpdateValue(pMissionInfo->ConditionKind, pMissionInfo->ConditionValue, conditionValue))
		{
			if (setType == CQuestManager::SET)
				afterValue = std::max(afterValue, value);
			else if (setType == CQuestManager::INCREASE)
				afterValue += value;
		}

		
		if (afterValue > prevValue)
		{
			BattlePass::ePassMissionState afterState = BattlePass::ePassMissionState::PROCESSING;
			
			if (pMissionInfo->TargetValue <= afterValue)
				afterState = BattlePass::ePassMissionState::COMPLETE;
			
			vecNeedUpdate.emplace_back(std::make_tuple(spMission->m_PassKind, spMission->m_Round, spMission->m_MissionKind, afterValue, afterState, spMission->m_RegistTime));
		}
		
	}
}

void CBattlePassContainer::AddPassPoint(CUser* const pUser, INT32 i32ItemKind, INT32 i32ItemCount)
{
	if (IS_NULL(pUser))
		return;

	BASE::ITEM_INFO* pItemInfo = BASE::GET_ITEM_DATA(i32ItemKind);
	if (IS_NULL(pItemInfo))
		return;

	if (pItemInfo->i32ITEM_TYPE != GAME::ITEM_TYPE_TICKET || pItemInfo->i32VALUE[0] != GAME::TICKET_BATTLEPASS_POINT_PIECE)
		return;

	// itemInfo Value4에 포인트 지급할 passkind가 등록되어있다.
	const auto passKind = pItemInfo->i32VALUE[3];

	const auto& spUserPass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spUserPass == nullptr)
		return;

	if (spUserPass->m_PassState != BattlePass::ePassState::PROCESSING)
		return;

	if (BattlePass::CBattlePassManager::Instance()->HasItemInVecGiveItem(passKind, i32ItemKind) == false)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,"BattlePass AddPassPoint Item data Invalid. [ItemKind = {}][PassKind = {}]", i32ItemKind, passKind);
		return;
	}

	//실질적인 포인트 지급 처리
	auto iItemCurCount = pUser->GetInventory().GetItemNum(i32ItemKind);

	const auto userCurPoint = spUserPass->GetPassPoint();
	const auto iMaxPoint = BattlePass::CBattlePassManager::Instance()->GetPassMaxPoint(spUserPass->m_PassKind, spUserPass->m_RewardGroupID);
	// 이미 MaxTier라면 제거한다.
	if (userCurPoint >= iMaxPoint)
	{
		NEW_FLATBUFFER(DB_ITEM_DELETE_REQ, pPacket);
		pPacket.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				return PROTOCOL::FLATBUFFERS::CreateDB_ITEM_DELETE_REQ(fbb,
					pUser->UID(),
					GLOBAL::GS_INFO.SVID,
					i32ItemKind,
					iItemCurCount,
					i32ItemCount,
					i32ItemKind);
			});
		SEND_DBA(pPacket);

		NEW_FLATBUFFER(GS_ITEM_USE_ACK, pPACKET);
		pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				return CreateGS_ITEM_USE_ACK(fbb, 0,
					PROTOCOL::FLATBUFFERS::CreateITEM(fbb, i32ItemKind, i32ItemCount, 0, 0, 0, 0),
					NULL, 0, 0, 0,
					PROTOCOL::FLATBUFFERS::CreateASSET(fbb,
						pUser->GetAssets().GetOil(), pUser->GetAssets().GetIron(), pUser->GetAssets().GetSilver(), pUser->GetAssets().GetGold()),
						PROTOCOL::FLATBUFFERS::CreateASSET(fbb, 0LL, 0LL, 0LL, 0LL),
						0, 0, 0, true);
			});
		SEND_ACTIVE_USER(pUser, pPACKET);

		// 아이템 제거 되었을 때 유저한테 알림 보내는건데 이거 텍키 필요함 흥순님 확인 필요
		//GLOBAL::NOTICE_MANAGER.SendAlarmMessage(pUser, _T("UI_SEASON_PASS_POINT_PIECE_UNABLE_2"), CNoticeManager::MessageBoxType::eAlram);

		return;
	}

	// 여기로 오면 진짜 로직 돌려
	const INT64 iAddPoint = static_cast<INT64>(i32ItemCount) * pItemInfo->i32VALUE[2];
	const INT64 iTotalPoint = userCurPoint + iAddPoint;

	const auto userPassType = spUserPass->GetPassType();

	CBattlePassHelper::P_BATTLEPASS_ADD_PASS_POINT(pUser, spUserPass->m_PassKind, spUserPass->m_Round, userPassType, iTotalPoint, i32ItemKind, i32ItemCount);
}

void CBattlePassContainer::AdjustSeasonalBuff(CUser* const pUser)
{
	if (pUser == nullptr)
		return;

	bool needUpdateBuff = false;

	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();
	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;

		const auto userPassTpye = pUserPass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : pUserPass->m_spPoint->m_PassType;
		if (userPassTpye == BattlePass::ePassType::FREE)
			continue;

		const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(passKind);
		if (passInfo == nullptr)
			continue;

		//여기 까지 왔으면 버프 보정!
		INT64 curGameTime = GLOBAL::GetGameTimeDueDay(0);
		INT64 curTime = GetDueDay_UTC(0);
		INT64 buffStartTime = curGameTime - (curTime - pUserPass->m_StartTime);
		INT64 buffEndTime = curGameTime + (pUserPass->m_EndTime - curTime);
		
		for (int i = 0; i < passInfo->vecSeasonalBuff.size(); ++i)
		{
			auto pItemInfo = BASE::GET_ITEM_DATA(passInfo->vecSeasonalBuff[i]);
			if (IS_NULL(pItemInfo))
				continue;

			if (pItemInfo->i32ITEM_TYPE != GAME::ITEM_TYPE_BUFF)
				continue;

			auto  buff_content_type = pItemInfo->GetBuffContentsType();
			INT32 buffLevel         = pItemInfo->i32VALUE[1];
			INT32 itemGroup         = pItemInfo->i32VALUE[2];
			INT64 workID            = 0;

			auto pBuffCtrl = pUser->GetBuffController();
			if (IS_NULL(pBuffCtrl))
				continue;

			auto& activeBuffCtrler = pUser->GetActiveBuffController(buff_content_type);
			auto& expireElem       = (0 < itemGroup) ? activeBuffCtrler.GetElemByGroup(itemGroup) : activeBuffCtrler.GetElem(pItemInfo->i32ITEM_KIND);
			if (expireElem.expireTime == buffEndTime)
				continue;

			if (0 < expireElem.kind)
			{
				auto pFindWork = pUser->Territory().FindWork([&](CWork* pWork)->bool { return pWork->GetCommandType() == GAME::eCOMMAND_TYPE::ITEM_PERIOD_USE && pWork->GetKind() == expireElem.kind; });
				workID = (IS_NOT_NULL(pFindWork)) ? pFindWork->GetIDX() : 0;

				auto pDisuseItemInfo = BASE::GET_ITEM_DATA(expireElem.kind);
				if (IS_NOT_NULL(pDisuseItemInfo))
				{
					for (int index = 0; index < pDisuseItemInfo->GetBuffElemCount(); ++index)
					{
						auto pElem = pDisuseItemInfo->GetBuffElem(index);
						if (IS_NULL(pElem))
						{
							continue;
						}

						pBuffCtrl->DeleteBuff(buff_content_type, pElem->UID, true);
					}

					if (pDisuseItemInfo->i32ITEM_KIND != pItemInfo->i32ITEM_KIND)
					{
						NEW_FLATBUFFER(DB_BUFF_UPDATE_REQ, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							auto buff = PROTOCOL::FLATBUFFERS::CreateBUFF(fbb,
								pUser->UID(),
								buff_content_type,
								pDisuseItemInfo->i32ITEM_KIND,
								expireElem.level,
								expireElem.startTime,
								GLOBAL::GetGameTimeDueDay(0));

							return PROTOCOL::FLATBUFFERS::CreateDB_BUFF_UPDATE_REQ(fbb,
								pUser->UID(),
								GLOBAL::GS_INFO.SVID,
								pDisuseItemInfo->i32ITEM_KIND,
								buff);
						});
						SEND_DBA(pPACKET);
					}
				}

				needUpdateBuff = true;
			}
		
			for (int index = 0; index < pItemInfo->GetBuffElemCount(); ++index)
			{
				auto pBuffElem = pItemInfo->GetBuffElem(index);
				if (IS_NULL(pBuffElem))
				{
					continue;
				}

				pBuffCtrl->CreateBuff(
					pItemInfo->GetBuffContentsType(),
					pBuffElem->buffProperty(),
					BASE::MAKE_BUFF_QUANTITY(pBuffElem, buffLevel, true),
					pBuffElem->CALCTYPE);

				needUpdateBuff = true;
			}

			NEW_FLATBUFFER(DB_WORK_ITEM_PERIOD_SET_REQ, pPACKET_ITEM);
			pPACKET_ITEM.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				return PROTOCOL::FLATBUFFERS::CreateDB_WORK_ITEM_PERIOD_SET_REQ(fbb,
					GLOBAL::GS_INFO.SVID,
					pUser->UID(),
					workID,
					GAME::eCOMMAND_TYPE::ITEM_PERIOD_USE,
					pItemInfo->i32ITEM_KIND,
					buffStartTime,
					buffEndTime,
					itemGroup,
					buffEndTime - buffStartTime,
					0);
			});
			SEND_DBA(pPACKET_ITEM);

			NEW_FLATBUFFER(DB_BUFF_SET_REQ, pPACKET_BUFF);
			pPACKET_BUFF.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				auto buff = PROTOCOL::FLATBUFFERS::CreateBUFF(fbb,
					pUser->UID(),
					pItemInfo->GetBuffContentsType(),
					pItemInfo->i32ITEM_KIND,
					buffLevel,
					buffStartTime,
					buffEndTime);

				return PROTOCOL::FLATBUFFERS::CreateDB_BUFF_SET_REQ(fbb,
					pUser->UID(),
					GLOBAL::GS_INFO.SVID,
					buff);
			});
			SEND_DBA(pPACKET_BUFF);

			NEW_FLATBUFFER(GS_ITEM_USE_ACK, pPACKET);
			pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
			{
				return PROTOCOL::FLATBUFFERS::CreateGS_ITEM_USE_ACK(fbb,
					0,
					PROTOCOL::FLATBUFFERS::CreateITEM(fbb, passInfo->vecSeasonalBuff[i], 0, 0, 0, buffEndTime - buffStartTime, buffLevel),
					NULL, buffStartTime, buffEndTime, expireElem.kind,
					PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pUser->GetAssets().GetOil(), pUser->GetAssets().GetIron(), pUser->GetAssets().GetSilver(), pUser->GetAssets().GetGold()),
					PROTOCOL::FLATBUFFERS::CreateASSET(fbb, 0LL, 0LL, 0LL, 0LL),
					0, 0, 0, true);
			});
			SEND_ACTIVE_USER(pUser, pPACKET);
		}
	}

	if (needUpdateBuff)
	{
		pUser->UserUpdateStats();
		pUser->SendBattlePower();
	}
}

INT32 CBattlePassHelper::GetWeekDay()
{
	INT64 i64CurTime = GetDueDay_UTC(0);
	struct tm tmTime;
	gmtime_s(&tmTime, &i64CurTime);
	return tmTime.tm_wday != 0 ? tmTime.tm_wday : 7;
}

void CBattlePassHelper::OnUserCreate(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	ASE_INSTANCE(pUser, CBattlePassContainer)->SetUserID(pUser->UID());

	ASE_INSTANCE(pUser, CBattlePassContainer)->SetDirty();
	CBattlePassHelper::CheckRefreshTime(pUser.get());
}

void CBattlePassHelper::OnUserLogin(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;
	
	CBattlePassHelper::CheckRefreshTime(pUser.get(), true);

	CheckPassPurchaseInApp(pUser.get(), GetDueDay_UTC(0));
}

void CBattlePassHelper::OnUserComeback(std::shared_ptr<CUser> const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;
	
	CBattlePassHelper::P_BATTLEPASS_LOAD({ pUser->UID() }, false);
}

void CBattlePassHelper::OnUserLevelUp(CUser* const pUser)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return;

	ASE_INSTANCE(pUser, CBattlePassContainer)->SetDirty();
	CBattlePassHelper::CheckRefreshTime(pUser);
}

void CBattlePassHelper::OnPurchaseComplete(CUser* const pUser, const INT32 productKind, INT64 checkTime)
{
	if (pUser == nullptr || pUser->IsPC() == false)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG, 
			"CBattlePassHelper::OnPurchaseComplete User is Null. [ProductKind = {}]", productKind);
		return;
	}


	// 배틀 패스와 관련 없은 상품 kind이면 그냥 return 하는게 맞다
	const BattlePass::BattlePassInfo::SharedPtr spPassInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfoByPackageKind(productKind);
	if (spPassInfo == nullptr)
		return;

	// 여기서 부터는 예외처리에 문제 생기면 로그 + 알림 보내야한다.
	
	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(spPassInfo->PassKind);
	if (spBattlePass == nullptr)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,  
			"CBattlePassHelper::OnPurchaseComplete BattlePass User Data Not Exist. [UserID = {}][ProductKind = {}][PassKind =  {}]", pUser->UID(), productKind, spPassInfo->PassKind);
		return;
	}
	
	// 이미 구매한 상품인지
	if (spBattlePass->m_spPoint != nullptr && spBattlePass->m_spPoint->m_PassType == BattlePass::ePassType::PAID)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,  
			"CBattlePassHelper::OnPurchaseComplete AlReady PurchaseComplete. [UserID = {}][ProductKind = {}][PassKind =  {}]", pUser->UID(), productKind, spPassInfo->PassKind);
		return;
	}

	// 만료된 이벤트 상품을 구매한 것인지
	if(spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,  
			"CBattlePassHelper::OnPurchaseComplete BattlePass Schedule is DayOff. [UserID = {}][ProductKind = {}][PassKind =  {}]", pUser->UID(), productKind, spPassInfo->PassKind);
		return;
	}

	const auto userCurPassPoint = spBattlePass->m_spPoint == nullptr ? 0 : spBattlePass->m_spPoint->m_curPoint;
	const auto userCurPassType = spBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : spBattlePass->m_spPoint->m_PassType;
	
	auto resultPoint = 0;

	if (userCurPassPoint >= static_cast<INT64>(spPassInfo->TierUpto) * BattlePass::TIER_TO_POINT)
		resultPoint = userCurPassPoint;
	else
		resultPoint = (static_cast<INT64>(spPassInfo->TierUpto) * BattlePass::TIER_TO_POINT) + (userCurPassPoint % BattlePass::TIER_TO_POINT);

	const CBattlePassPoint::RollbackPassInfo rollbackPoint = std::make_tuple(spBattlePass->m_PassKind, spBattlePass->m_Round, userCurPassType, userCurPassPoint);

	P_BATTLEPASS_PURCHASE_Pass_By_WAS(pUser, spBattlePass->m_PassKind, spBattlePass->m_Round, spBattlePass->m_RewardGroupID, BattlePass::ePassType::PAID, resultPoint, rollbackPoint);
}

bool CBattlePassHelper::CheckRefreshTime(CUser* const pUser, bool sendClient)
{
	if (pUser == nullptr || pUser->IsPC() == false)
		return false;

	// 서버 대전넘어 갈 때는 클라에서 데이터 초기화하는 처리 필요함
	if (GLOBAL::IsBattleRoyalServer() || GLOBAL::IsServerWarsServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
		return false;

	// 유저 ID를 세팅하기 전이라면 진행 x
	// 휴면 복귀(or 서버 이동) 후 데이터 로드 완료했을 때,  캐릭터 생성했을 때, 서버 기동 후 데이터 로드 완료했을 때 UserID 세팅 ( SetUserID() 참조로 확인 )
	if (IS_ZERO( ASE_INSTANCE(pUser, CBattlePassContainer)->GetUserID() ) )
		return false;
	
	INT64 curTime = GetDueDay_UTC(0);
	INT64 refreshTime = ASE_INSTANCE(pUser, CBattlePassContainer)->GetRefreshTime();
	bool needNFY = false;

	if (false == CheckSameDay_UTC(refreshTime, curTime))
	{
		needNFY |= CBattlePassHelper::CheckPassExpired(pUser, curTime);
		needNFY |= CBattlePassHelper::CheckPassOpen(pUser, curTime);
		needNFY |= CBattlePassHelper::CheckPassMissionExpire(pUser, curTime);
		needNFY |= CBattlePassHelper::CheckPassMissionOpen(pUser, curTime);
		
		ASE_INSTANCE(pUser, CBattlePassContainer)->UpdateQuestCacheMap();
		ASE_INSTANCE(pUser, CBattlePassContainer)->SetRefreshTime(curTime);
		ASE_INSTANCE(pUser, CBattlePassContainer)->AdjustSeasonalBuff(pUser);
	}

	if (sendClient || needNFY)
		Send_GS_BATTLEPASS_INFO_NFY(pUser);
	
	return needNFY;
}

bool CBattlePassHelper::CheckPassExpired(CUser* const pUser, const INT64 curTime)
{
	if (pUser == nullptr)
		return false;

	bool result = false;
	CBattlePass::UpdateScheduleInfos expiredPass;
	std::vector<BASE::REWARDITEM> vecSetItem;
	std::vector<BASE::REWARDITEM> vecRollbackItem;
	
	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();

	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		const auto pPassInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(pUserPass->m_PassKind);
		if (pPassInfo == nullptr)
			continue;

		if (curTime < pUserPass->m_StartTime || curTime >= pUserPass->m_EndTime)
		{
			if (pUserPass->m_PassState == BattlePass::ePassState::PROCESSING)
			{
				result = true;
				expiredPass.emplace_back(std::make_tuple(pUser->UID(), passKind, pUserPass->m_Round, pUserPass->m_RewardGroupID, pUserPass->m_StartTime, pUserPass->m_EndTime, BattlePass::ePassState::DAYOFF));
				
				// 이 지급 로직 프로시저로 빼야한다.
				//구매 했으면 Item지급
				//
				if (pPassInfo->vecGiveItem.size() <= 0)
					continue;
	
				const INT32 kind = pPassInfo->vecGiveItem[0].Kind;
				const INT32 count = pPassInfo->vecGiveItem[0].Count;

				const auto passType = pUserPass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : pUserPass->m_spPoint->m_PassType;
				
				if (passType == BattlePass::ePassType::PAID)
				{
					//무조건 40개를 유지 해야된다..
					vecSetItem.emplace_back(BASE::REWARDITEM(kind, count));
				}
				//만약에 전꺼를 안샀으면 지워야한다..!
				else if (passType == BattlePass::ePassType::FREE)
				{
					auto itemCurCount = pUser->GetInventory().GetItemNum(kind);
					if (itemCurCount > 0)
						vecSetItem.emplace_back(BASE::REWARDITEM(kind, 0));
				}

				auto& userInven = pUser->GetInventory();
				vecRollbackItem.emplace_back(BASE::REWARDITEM(kind, userInven.GetItemNum(kind)));
			}
		}
	}

	if (expiredPass.empty() == false)
	{
		P_BATTLEPASS_SCHEDULE_UPSERT_TV(pUser, curTime, expiredPass, vecSetItem, vecRollbackItem);
		return result;
	}

	return result;
}

bool CBattlePassHelper::CheckPassOpen(CUser* const pUser, const INT64 curTime)
{
	if (pUser == nullptr)
		return false;

	//const auto userCreateTime = pUser->GetCreateTime();
	
	auto& allScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetAllSchedule();
	CBattlePass::UpdateScheduleInfos registPass;
	
	std::vector<BASE::REWARDITEM> vecSetItem, vecRollbackItem;
	
	for (const auto& [passKind, passSchedule] : allScheduleInfo)
	{
		if (passSchedule == nullptr)
			continue;
		
		const auto pScheduleInfo = passSchedule.get();
		if (pScheduleInfo == nullptr)
			continue;
		
		INT64 startTime = 0;
		INT64 endTime = 0;
		INT32 Round = 1;
		INT32 RewardGroup = 0;
		// 2
		// 해당 패스 데이터가 존재할 경우, 아직 진행 중인지, 대기 상태인지 체크
		if(auto pUserPass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind); pUserPass != nullptr)
		{
			// 아직 진행 중인 이벤트라면 검사 x
			if (pUserPass->m_PassState == BattlePass::ePassState::PROCESSING)
				continue;

			// 만료된 이벤트지간 휴식 기간이라면 검사 x
			if(curTime < pUserPass->m_EndTime + static_cast<INT64>(SECOND_PER_DAY) * pScheduleInfo->Dayoff)
				continue;
			
			Round = pUserPass->m_Round + 1;


			if (pUserPass->m_spPoint != nullptr)
			{
				pUserPass->m_spPoint->m_curPoint = 0;
				pUserPass->m_spPoint->m_PassType = BattlePass::ePassType::FREE;
				pUserPass->m_spPoint->m_Round = Round;
				pUserPass->m_setCheckWeek.clear();
			}

		}

		// 스케쥴 공통으로 open 될 수 있는 사령부 최소레벨을 검사한다.
		if(pUser->GetCastleLevel() < passSchedule->MinLevel)
			continue;

		// 유저 단위로 따로 진행되는 스케쥴 타입
		if (passSchedule->ScheduleType == BattlePass::eScheduleType::CASTLELEVEL)
		{
			// CASTLELEVEL 의 경우에는 조건 맞으면 바로 열리는 패스라 지금 기준으로 서버 나이 충족햇는지 검사
			const INT32 serverAge = GetServerAge(curTime);
			if (serverAge < pScheduleInfo->MinServerAge)
				continue;
			
			if (pUser->GetCastleLevel() >= passSchedule->ScheduleValue)
			{
				//오늘 UTC 00시
				startTime = GetFixedTime_UTC(curTime);
				endTime = startTime + (static_cast<INT64>(SECOND_PER_DAY) * passSchedule->ProgressDay);
				RewardGroup = BattlePass::CBattlePassManager::Instance()->GetRewardGroupIndex(passKind, startTime);
			}
		}
		// NDT에 설정된 UTC 단위로 진행되는 스케쥴 타입
		// 여기서는 서버나이도 검사해야한다.
		else if (passSchedule->ScheduleType == BattlePass::eScheduleType::MONTHLY)
		{
			struct tm tm_MatchTime;
			gmtime_s(&tm_MatchTime, &curTime);

			const auto curYear = GetCurrentUTCYear(curTime) + START_YEAR;
			const auto curMonth = GetCurrentUTCMonth(curTime) + 1;
			
			const INT64  utcLastDayOfMonth = BattlePass::CBattlePassManager::Instance()->GetUTCLastDayOfMonth(curTime);

			// yday, YearDay : 1월 1일 기준으로 며칠차인지
			
			const INT32 curYearDay = tm_MatchTime.tm_yday;

			const INT32 startYearDay = GetUTCYearDay(GetUTCTime(curYear, curMonth, passSchedule->ScheduleValue, 0));
			const INT32 offDay = curYearDay - startYearDay;
			const INT32 endYearDay = (0 < passSchedule->ProgressDay)
				? startYearDay + passSchedule->ProgressDay                                     // ProgressDay가 입력되어있다면 ProgressDay로 endDay 계산
				: (GetUTCYearDay(utcLastDayOfMonth - static_cast<INT64>(SECOND_PER_DAY)) + 1); // 연말(12/31,24시)의 경우 GetUTCYearDay가 0으로 나와서 제대로 계산이 안됨
	
			if ( startYearDay <= curYearDay && curYearDay < endYearDay )
			{
				startTime = GetFixedTime_UTC(curTime) - (static_cast<INT64>(SECOND_PER_DAY) * offDay);
				endTime = (passSchedule->ProgressDay != 0) ? startTime + ((static_cast<INT64>(SECOND_PER_DAY) * passSchedule->ProgressDay)) : utcLastDayOfMonth;
				
				RewardGroup = BattlePass::CBattlePassManager::Instance()->GetRewardGroupIndex(passKind, startTime);

				// utc로 진행되는 이벤트이다보니, 현재 기준 서버나이가 아닌 시작 시간을 기준으로 서버나이 검사
				const INT32 serverAge = GetServerAge(startTime);
				if (serverAge < pScheduleInfo->MinServerAge)
					continue;
			}

		}

		if (startTime < 0 || endTime < 0 || curTime < startTime || endTime < curTime)
			continue;

		// UserID, PassKind, Round, RewardGroup, StartTime, EndTime, PassState
		registPass.emplace_back(std::make_tuple(pUser->UID(), passKind, Round, RewardGroup, startTime, endTime, BattlePass::ePassState::PROCESSING));
	}

	if (registPass.empty() == false)
	{
		P_BATTLEPASS_SCHEDULE_UPSERT_TV(pUser, curTime, registPass, vecSetItem, vecRollbackItem);
		return true;
	}

	return false;
}

// RegistQuest에서 한번 등록을 시켜둔 상태이다. 
bool CBattlePassHelper::CheckPassMissionOpen(CUser* const pUser, const INT64 curTime)
{
	// 배틀패스가 처음 열렸을 때 등록되어야하는 미션은 이미 등록되어 있는 상태다. ( 시즌은 검사 x )
	// 일간 주간 단위로 새로 열려야하는 미션이 있는지 검사할 것

	if (pUser == nullptr)
		return false;

	// PassKind, Round, MissionKind, CurValue, Statet
	CBattlePassMission::UpdateMissionInfos registMissions;
	
	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();
	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		// 지금 진행 중인 패스
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;

		const auto userCastleLevel = pUser->GetCastleLevel();
		
		// 일간 검사
		if( const auto typeMissionMap = BattlePass::CBattlePassManager::Instance()->GetMissionInfosByType(passKind); typeMissionMap != nullptr)
		{
			// 일간 미션은 오늘이 무슨 요일인지가 중요하다
			const INT32 curWeekDay = CBattlePassHelper::GetWeekDay();
			for (const auto& [key, wpMission] : GAME::for_range(typeMissionMap->equal_range(BattlePass::ePassMissionType::DAY)))
			{
				const auto pMissionInfo = wpMission.lock();
				if (pMissionInfo == nullptr)
					continue;

				// 오늘 열릴 수 있는 미션이 아니라면 
				if (pMissionInfo->MissionScheduleValue != 0 && pMissionInfo->MissionScheduleValue != curWeekDay)
					continue;

				// 사령부레벨이 부족하다면
				if (pMissionInfo->IsCastleLevelInRange(userCastleLevel) == false)
					continue;

				// 이미 등록되어 있다면 해당 미션의 등록시간도 검사해야한다.
				if(const auto userMission = pUserPass->m_MissionMap.find(pMissionInfo->Missionkind); userMission != pUserPass->m_MissionMap.end() )
				{
					// 이미 같은 날에 등록된 미션이라면 기존 미션 사용
					if (true == CheckSameDay_UTC(userMission->second->m_RegistTime, curTime))
						continue;
				}
				
				registMissions.emplace_back(std::make_tuple(pUserPass->m_PassKind, pUserPass->m_Round, pMissionInfo->Missionkind, 0, BattlePass::ePassMissionState::PROCESSING, curTime));
			}

			// 주간 검사
			// 주간 미션은 오늘이 패스를 진행한지 몇 주가 되었는지가 중요하다
			const INT32 passWeek = pUserPass->GetPassWeek();
			for (const auto& [key, wpMission] : GAME::for_range(typeMissionMap->equal_range(BattlePass::ePassMissionType::WEEK)))
			{
				const auto pMissionInfo = wpMission.lock();
				if (pMissionInfo == nullptr)
					continue;

				// 이미 해당 주차 미션 세팅을 했다면, 검사 x
				if (pUserPass->m_setCheckWeek.find(pMissionInfo->MissionScheduleValue) != pUserPass->m_setCheckWeek.end())
					continue;

				// 더 높은 주 차의 미션은 세팅 x
				if (passWeek < pMissionInfo->MissionScheduleValue)
					continue;
				
				// 사령부레벨이 부족하다면
				if (pMissionInfo->IsCastleLevelInRange(userCastleLevel) == false)
					continue;

				// 주간 미션의 경우, 한 배틀패스에서 이미 진행한 주간 미션이 다시 열리는 경우가 없기 떄문에 값이 존재하면 continue;
				if (const auto userMission = pUserPass->m_MissionMap.find(pMissionInfo->Missionkind); userMission != pUserPass->m_MissionMap.end())
					continue;
				
				registMissions.emplace_back(std::make_tuple(pUserPass->m_PassKind, pUserPass->m_Round, pMissionInfo->Missionkind, 0, BattlePass::ePassMissionState::PROCESSING, curTime));
			}
		}	
	}

	if (registMissions.empty() == false)
	{
		P_BATTLEPASS_MISSION_UPSERT_TV(pUser, registMissions, true);
		return true;
	}
	
	return false;
}

bool CBattlePassHelper::CheckPassMissionExpire(CUser* const pUser, const INT64 curTime)
{
	if (pUser == nullptr)
		return false;

	//const INT32 curWeekDay = CBattlePassHelper::GetWeekDay();
	
	// PassKind, Round, MissionKind, CurValue, Statet
	CBattlePassMission::UpdateMissionInfos registMissions;

	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();

	// 여기서는 내가 가지고 있는 미션 중에서 만료 처리해야하는 미션이 있는지 확인하자( 일일 미션만 해당된다. )
	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;

		for(const auto& [MissionKind, userMission] : pUserPass->m_MissionMap)
		{
			if (userMission == nullptr)
				continue;

			if (userMission->m_MissionState == BattlePass::ePassMissionState::EXPIRE)
				continue;

			const auto missionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(pUserPass->m_PassKind, MissionKind);
			if(missionInfo == nullptr)
			{
				// 예외처리로 추가함. ndt에 없다면 체크하는게 이상한건데?
				// 상태만 변경하자
				registMissions.emplace_back(std::make_tuple(pUserPass->m_PassKind, pUserPass->m_Round, userMission->m_MissionKind, userMission->m_CurValue, BattlePass::ePassMissionState::EXPIRE, userMission->m_RegistTime));
				continue;
			}

			// 일일 미션일 경우, 
			if(missionInfo->MissionScheduleType == BattlePass::ePassMissionType::DAY)
			{
				//현재 시간과 검사했을 때 Day가 다르면 만료 처리
				if (false == CheckSameDay_UTC(userMission->m_RegistTime, curTime))
					registMissions.emplace_back(std::make_tuple(userMission->m_PassKind, userMission->m_Round, userMission->m_MissionKind, userMission->m_CurValue, BattlePass::ePassMissionState::EXPIRE, userMission->m_RegistTime));
			}

		}
		
	}

	if (registMissions.empty() == false)
	{
		P_BATTLEPASS_MISSION_UPSERT_TV(pUser, registMissions);
		return true;
	}

	return false;
}

void CBattlePassHelper::CheckPassPurchaseInApp(CUser* const pUser, const INT64 curTime)
{
	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();

	std::unordered_map<INT32, INT64> purchase_check;
	
	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;

		const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(pUserPass->m_PassKind);
		if (passInfo == nullptr)
			continue;

		if(IS_ZERO(passInfo->PackageKind))
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;
		
		const auto userPassType = pUserPass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : pUserPass->m_spPoint->m_PassType;
		if (userPassType == BattlePass::ePassType::PAID)
			continue;

		// 해당 상품을 배플패스 시작한 이후로 구매한적이 있는지 검사한다.
		purchase_check.emplace(std::make_pair(passInfo->PackageKind, pUserPass->m_StartTime));
	}

	INT64 userID = pUser->UID();
	//const auto nServerID = pUser->GetServerID();
	for (const auto& pc : purchase_check)
	{
		INT32 packind = pc.first;
		INT64 regTime = pc.second;

		::OnRemoteDBA([=](INT16 ssnid)->RECV_RESULT
			{
				BEGIN_GDB_QUERY_AUTO(P_PURCHASE_INAPP_CHECK);
				pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
				pQuery->m_nUserID = userID;
				pQuery->m_nItemKind = packind;
				pQuery->m_tmRegDate = TLDB::TIME_UTC2DB(regTime);
				pQuery->m_nGroupCode = BattlePass::BATTLEPASS_PACKAGE_GROUPCODE;
				RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

				::OnRemoteGAME(ssnid, [=, SET_1 = pQuery->GetSET_1()]()->RECV_RESULT
				{
					if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
					{
						if (SET_1.size() > 0 && SET_1[0].m_nItemCount > 0)
						{
							// 먼가 구매를 한 기록이 있다!
							// 이 로직을 태우기 전에 이미 구매를 했는지, 패스가 진행중인지 다 검사를했기때문에
							// 결과에서 먼가 구매한 기록이 있다고하면 서버 인메모리 갱신을 해주자
							OnPurchaseComplete(pUser, packind, curTime);
							
							return  RECV_OK;
						}
					}

					return RECV_OK;
				});

				END_GDB_QUERY();

				return RECV_OK;
			});
	}
	
}

// newData을 컨테이너에 처음 등록할 때 사용한다. 이 때 등록되어있어야하는 데이터만 우선 세팅하고
// Mission을 Load해오면서 이미 존재하면 덮어씌우는 방식
bool CBattlePassHelper::RegistQuest(CUser* const pUser, CBattlePass::SharedPtr newData)
{
	if (newData == nullptr)
		return false;

	if (pUser == nullptr)
		return false;

	newData->m_MissionMap.clear();

	if (newData->m_PassState == BattlePass::ePassState::DAYOFF)
		return false;

	const auto curTime = GetDueDay_UTC(0);
	CBattlePassMission::UpdateMissionInfos vecNeedUpdate;
	
	// 나는 여기서 PassKind만 넘길거다
	const BattlePass::BattlePassScheduleInfo::SharedPtr spPassScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(newData->m_PassKind);
	if (spPassScheduleInfo == nullptr)
		return false;

	const auto missionInfoMap = BattlePass::CBattlePassManager::Instance()->GetMissionInfos(spPassScheduleInfo->PassKind);
	if (missionInfoMap == nullptr)
		return false;

	const auto userCastleLevel = pUser->GetCastleLevel();
	
	// 여기에는 해당 배틀패스에서 열릴 수 있는 모든 퀘스트들 담겨있다.
	for (const auto& [_, pMissionInfo] : *missionInfoMap)
	{
		if (pMissionInfo == nullptr)
			continue;

		auto spMission = std::make_shared<CBattlePassMission>();
		if (spMission == nullptr)
			throw std::bad_alloc();

		switch (pMissionInfo->MissionScheduleType)
		{
		case BattlePass::ePassMissionType::NONE:
		case BattlePass::ePassMissionType::END:
			continue;
		case BattlePass::ePassMissionType::DAY:
			if (CBattlePassHelper::GetWeekDay() != pMissionInfo->MissionScheduleValue)
				continue;
			break;
		case BattlePass::ePassMissionType::WEEK:
			if (spPassScheduleInfo->ScheduleType != BattlePass::eScheduleType::CASTLELEVEL)
				continue;
			
			if (newData->GetPassWeek() != pMissionInfo->MissionScheduleValue)
				continue;
			break;
		default: break;
		}

		if (pMissionInfo->IsCastleLevelInRange(userCastleLevel) == false)
			continue;
		
		spMission->m_PassKind = pMissionInfo->PassKind;
		spMission->m_MissionKind = pMissionInfo->Missionkind;
		spMission->m_ConditionKind = pMissionInfo->ConditionKind;
		spMission->m_CurValue = 0;
		spMission->m_Round = newData->m_Round;
		spMission->m_MissionState = BattlePass::ePassMissionState::PROCESSING;
		spMission->m_RegistTime = curTime;
		
		newData->m_MissionMap.emplace(spMission->m_MissionKind, spMission);

		vecNeedUpdate.emplace_back(std::make_tuple(spMission->m_PassKind, spMission->m_Round, spMission->m_MissionKind, spMission->m_CurValue, spMission->m_MissionState, spMission->m_RegistTime));
	}

	if (false == vecNeedUpdate.empty())
	{
		CBattlePassHelper::P_BATTLEPASS_MISSION_UPSERT_TV(pUser, vecNeedUpdate, true);
	}
	
	return true;
}

void CBattlePassHelper::OnQuestEvent(INT64 userID, INT32 condKind, INT64 condValue, INT64 value, CQuestManager::SETTYPE settype)
{
	auto pUser = CUserManager::Instance()->FindByUID(userID);
	if (IS_NULL(pUser))
		return;

	if (pUser == nullptr || pUser->IsPC() == false)
		return;
	
	if (GLOBAL::IsBattleRoyalServer() || GLOBAL::IsServerWarsServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
		return;
	
	CheckRefreshTime(pUser);
	
	CBattlePassMission::UpdateMissionInfos vecNeedUpdate;

	// UpdateEvent에서 값만 변경된 녀석들이 있을 수 도 있고, 진행중 -> 완료로 변경된 녀석들이 있을 수 있다 => 캐싱 데이터 갱신이 필요하다.
	// 그런데 여기서 캐싱 데이터 갱신하는 순간 갱신 부하 엄청 발생.
	ASE_INSTANCE(pUser, CBattlePassContainer)->UpdateEvent(condKind, condValue, value, settype, vecNeedUpdate);


	if (false == vecNeedUpdate.empty())
	{
		CBattlePassHelper::P_BATTLEPASS_MISSION_UPSERT_TV(pUser, vecNeedUpdate, false, true);
	}
}

bool CBattlePassHelper::Send_GS_BATTLEPASS_INFO_NFY(CUser* const pUser, const INT64 passid)
{
	if (IS_ACTIVE_USER(pUser) == false)
		return false;

	NEW_FLATBUFFER(GS_BATTLEPASS_INFO_NFY, pPACKET);
	pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{	
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::BATTLEPASS_USER>>	vecUserInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::BATTLEPASS_REWARD>>  vecRewardInfos;
		std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::BATTLEPASS_MISSION>> vecMissionInfos;


		auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();

		for (const auto& [passKind, passSchedule] : spBattlePass)
		{
			const auto& pUserPass = passSchedule->pBattlePass;
			if (pUserPass == nullptr)
				continue;

			if (pUserPass->m_PassState != BattlePass::ePassState::PROCESSING)
				continue;

			vecUserInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_USER(fbb
				,pUserPass->m_PassKind
				,pUserPass->m_Round
				,pUserPass->m_RewardGroupID
				,pUserPass->m_spPoint == nullptr ? 0 : pUserPass->m_spPoint->m_curPoint
				,pUserPass->m_StartTime
				,pUserPass->m_EndTime
				,pUserPass->m_spPoint == nullptr ? static_cast<INT16>(BattlePass::ePassType::FREE) : static_cast<INT16>(pUserPass->m_spPoint->m_PassType)
			));

			auto& pUserRewardMap = pUserPass->m_RewardMap;
			for (const auto& pair_reward : pUserRewardMap)
			{
				auto& pRewardInfo = pair_reward.second;
				if (pRewardInfo == nullptr)
					continue;

				vecRewardInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_REWARD(fbb
					, pUserPass->m_PassKind
					, pUserPass->m_Round
					, pRewardInfo->m_Tier
					, static_cast<INT16>(pRewardInfo->m_RewardType)
				));
			}

			//다른 요일 미션도 보내주고 있어서 CheckRefresh에서 만료된 미션 정리 필요
			auto& pUserMissionMap = pUserPass->m_MissionMap;
			for (const auto& mission : pUserMissionMap)
			{
				auto& pMission = mission.second;
				if (pMission == nullptr)
					continue;
				
				vecMissionInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_MISSION(fbb
					, pUserPass->m_PassKind
					, pUserPass->m_Round
					, pMission->m_MissionKind
					, pMission->m_CurValue
					,static_cast<INT16>(pMission->m_MissionState)
				));
			}
		}

		return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_INFO_NFY(fbb, fbb.CreateVector(vecUserInfos), fbb.CreateVector(vecRewardInfos), fbb.CreateVector(vecMissionInfos));
	});
	SEND_ACTIVE_USER(pUser, pPACKET);
	return true;
}

bool CBattlePassHelper::OnRecvGS_BATTLEPASS_MISSION_COMPLETE_REQ(CUser* pUser, const void* pData)
{
	if (IS_NULL(pUser))
		return false;

	const auto pReq = PROTOCOL::FLATBUFFERS::GetGS_BATTLEPASS_MISSION_COMPLETE_REQ(pData);
	if (IS_NULL(pReq))
		return false;

	if (GLOBAL::IsBattleRoyalServer() || GLOBAL::IsServerWarsServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
		return false;
	
	const auto passKind = pReq->PassKind();
	const auto missionKind = pReq->MissionKind();

	if (IS_ZERO(passKind) || IS_ZERO(missionKind))
		return false;

	const auto userBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (IS_NULL(userBattlePass))
		return false;

	const BattlePass::BattlePassScheduleInfo::SharedPtr spScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(passKind);
	if (spScheduleInfo == nullptr)
		return false;
	
	// 종료된 이벤트 미션의 보상 받기 불가능
	if (userBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
		return false;

	const BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(passKind, missionKind);
	if (spMissionInfo == nullptr)
		return false;
	
	const auto userMission = userBattlePass->m_MissionMap.find(missionKind);
	if (userMission == userBattlePass->m_MissionMap.end())
		return false;

	if (userMission->second->m_MissionState != BattlePass::ePassMissionState::COMPLETE)
		return false;

	const auto userPassType = userBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : userBattlePass->m_spPoint->m_PassType;
	const auto userCurPoint = userBattlePass->m_spPoint == nullptr ? 0 : userBattlePass->m_spPoint->m_curPoint;
	const auto passMaxPoint = static_cast<INT64>(BattlePass::CBattlePassManager::Instance()->GetPassMaxPoint(userBattlePass->m_PassKind, userBattlePass->m_RewardGroupID));
	
	INT64 i64GetPointBuffPoint = BuffAssistant::GetRevisedTotalOnUser(pUser->UID(), spMissionInfo->RewardPoint, BuffContents::eBUFF_STAT_KIND::AMOUNT_SEASONPASS_POINT);

	const bool needPointBuff = BattlePass::CBattlePassManager::Instance()->CheckPassBuffStat(passKind, BuffContents::eBUFF_STAT_KIND::AMOUNT_SEASONPASS_POINT) == true && userPassType == BattlePass::ePassType::PAID;

	const INT64 i64GetPoint = needPointBuff ? i64GetPointBuffPoint : spMissionInfo->RewardPoint;

	const auto resultPoint = userCurPoint >= passMaxPoint ? userCurPoint : userCurPoint + i64GetPoint;

	
	// 여기까지 왔으면 보상 세팅 가능한디
	const CBattlePassMission::UpdateMissionInfo needUpdateMission = std::make_tuple(userBattlePass->m_PassKind, userBattlePass->m_Round, userMission->second->m_MissionKind,userMission->second->m_CurValue, BattlePass::ePassMissionState::REWARDED, userMission->second->m_RegistTime);
	const CBattlePassPoint::UpdatePassInfo updatePassPoint = std::make_tuple(userBattlePass->m_PassKind, userBattlePass->m_Round, resultPoint);

	const CBattlePassMission::UpdateMissionInfo rollBackMission = std::make_tuple(userBattlePass->m_PassKind, userBattlePass->m_Round, userMission->second->m_MissionKind, userMission->second->m_CurValue, userMission->second->m_MissionState, userMission->second->m_RegistTime);
	const CBattlePassPoint::UpdatePassInfo rollBackPassPoint = std::make_tuple(userBattlePass->m_PassKind, userBattlePass->m_Round, userCurPoint);
	
	P_BATTLEPASS_MISSION_UPSERT_TV_By_MissionComplete(pUser, needUpdateMission, updatePassPoint, rollBackMission, rollBackPassPoint);
	
	return true;
}

bool CBattlePassHelper::OnRecvGS_BATTLEPASS_TIER_REWARD_REQ(CUser* pUser, const void* pData)
{
	if (IS_NULL(pUser))
		return false;

	auto pReq = PROTOCOL::FLATBUFFERS::GetGS_BATTLEPASS_TIER_REWARD_REQ(pData);
	if (IS_NULL(pReq))
		return false;

	const auto passKind = pReq->PassKind();
	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
		return false;

	if(spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
		return false;

	// 값 자체가 없다면 받을 수 있는 보상
	if (spBattlePass->m_spPoint == nullptr)
		return false;
	
	CBattlePassReward::UpdateRewardInfos needUpdateRewards;

	// 내 포인트도 검사해야한다.
	for (size_t i = 0; i < pReq->Tier()->Length(); ++i)
	{
		const auto reqTier = pReq->Tier()->Get(i);
		const auto rewardKey = std::make_pair(reqTier->Tier(), static_cast<BattlePass::eRewardType>(reqTier->RewardType()));
		const auto totalKey = std::make_tuple(reqTier->Kind(), reqTier->Round(), reqTier->Tier(), static_cast<BattlePass::eRewardType>(reqTier->RewardType()));

		const auto userReward = spBattlePass->m_RewardMap.find(rewardKey);
		if(userReward != spBattlePass->m_RewardMap.end())
			return false;

		// 내가 가지고 있는 포인트가 부족하다면 fail
		if ( static_cast<INT64>(reqTier->Tier()) * BattlePass::TIER_TO_POINT > spBattlePass->m_spPoint->m_curPoint)
			return false;

		// 요청한 보상이 유료 보상인데, 내 패스 등급이 PAID가 아니라면 fail
		if(static_cast<BattlePass::eRewardType>(reqTier->RewardType()) == BattlePass::eRewardType::PAID && spBattlePass->m_spPoint->m_PassType != BattlePass::ePassType::PAID)
			return false;

		// 이미 요청 할 데이터에 담아있다면 return 처리 필요. 클라 이슈 확인 전까지는 우선 서버에서 걸러서 보내주자
		//if (needUpdateRewards.find(totalKey) != needUpdateRewards.end())
		//	continue;
		
		auto [iter, result] = needUpdateRewards.emplace(totalKey);
		// 중복 요청을 제거하고 보낼 경우, 에러 추적이 어려워서 우선 파일로그로 기록 저장
		if (result == false)
		{
			LOGGER_WARN(CONST_BATTLEPASS_LOG,
				"Duplicate Reward Req!!!! ServerID = {}, UserID = {}, PassKind = {}, Round = {}, Tier = {}, RewardType = {}",
				GLOBAL::GS_INFO.SVID, pUser->UID(), reqTier->Kind(), reqTier->Round(), reqTier->Tier(), static_cast<INT32>(reqTier->RewardType()));
		}
	}

	if(false == needUpdateRewards.empty())
	{
		CBattlePassHelper::P_BATTLEPASS_REWARD_SET_TV(pUser, passKind, needUpdateRewards);
	}
	
	return true;
}

bool CBattlePassHelper::OnRecvGS_BATTLEPASS_TIER_PURCHASE_REQ(CUser* pUser, const void* pData)
{
	if (IS_NULL(pUser))
		return false;

	const auto pReq = PROTOCOL::FLATBUFFERS::GetGS_BATTLEPASS_TIER_PURCHASE_REQ(pData);
	if (IS_NULL(pReq))
		return false;

	const auto passKind = pReq->Passkind();
	const auto reqCurPoint = pReq->CurPoint();
	const auto reqTargetTier = pReq->TargetTier(); // 클라에서 요청한 티어
	const auto NeedGem = pReq->NeedGem(); // 필요한 젬
	const auto ClientGem = pReq->ClientGem(); // 클라이언트 젬

	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
		return false;

	if(spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
		return false;

	const auto userCurPassPoint = spBattlePass->m_spPoint == nullptr ? 0 : spBattlePass->m_spPoint->m_curPoint;
	const auto userPassType = spBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : spBattlePass->m_spPoint->m_PassType;
	if (spBattlePass->m_spPoint != nullptr)
	{
		if(reqCurPoint != userCurPassPoint)
			return false;
	}

	// 이게 진짜 올리고 싶은 최종 포인트
	const auto targetPoint = static_cast<INT64>(BattlePass::TIER_TO_POINT * reqTargetTier) + userCurPassPoint % BattlePass::TIER_TO_POINT;
	if(userCurPassPoint >= targetPoint)
		return false;

	const auto calcNeedGem = BattlePass::CBattlePassManager::Instance()->GetNeedGem(userCurPassPoint, targetPoint);
	if (calcNeedGem != NeedGem)
		return false;

	if (ClientGem < calcNeedGem)
		return false;

	const CBattlePassPoint::RollbackPassInfo rollBackPoint = make_tuple(spBattlePass->m_PassKind, spBattlePass->m_Round, userPassType, userCurPassPoint);
	
	// 젬소모는 프로시저 호출하는 수 밖에 없다. 클라에서 잘 막아줘야한다.
	P_BATTLEPASS_PURCHASE_Tier(pUser, spBattlePass->m_PassKind, spBattlePass->m_Round, spBattlePass->m_RewardGroupID, userPassType, targetPoint, calcNeedGem, BattlePass::PURCHASE_TIER_ID, userCurPassPoint, rollBackPoint);
	return true;
}

bool CBattlePassHelper::OnRecvGS_BATTLEPASS_PASS_PURCHASE_REQ(CUser* pUser, const void* pData)
{
	if (IS_NULL(pUser))
		return false;

	const auto pReq = PROTOCOL::FLATBUFFERS::GetGS_BATTLEPASS_PASS_PURCHASE_REQ(pData);
	if (IS_NULL(pReq))
		return false;

	const auto PassKind = pReq->PassKind();
	//const auto Type = pReq->Type(); // 어떤 
	const auto ClientGem = pReq->ClientGem();

	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
	if (spBattlePass == nullptr)
		return false;

	if (spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
		return false;

	if (spBattlePass->m_spPoint != nullptr && spBattlePass->m_spPoint->m_PassType == BattlePass::ePassType::PAID)
		return false;

	const auto userCurPassPoint = spBattlePass->m_spPoint == nullptr ? 0 : spBattlePass->m_spPoint->m_curPoint;
	
	const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(PassKind);
	if(passInfo == nullptr)
		return false;

	if(IS_ZERO( passInfo->PackageKind))
	{
		auto resultPoint = 0;

		const CBattlePassPoint::RollbackPassInfo rollbackPoint = std::make_tuple(spBattlePass->m_PassKind, spBattlePass->m_Round, BattlePass::ePassType::FREE, userCurPassPoint);
				
		if (userCurPassPoint >= static_cast<INT64>(passInfo->TierUpto) * BattlePass::TIER_TO_POINT)
			resultPoint = userCurPassPoint;
		else
			resultPoint = (static_cast<INT64>(passInfo->TierUpto) * BattlePass::TIER_TO_POINT) + (userCurPassPoint % BattlePass::TIER_TO_POINT);

		// 젬소모는 프로시저 호출하는 수 밖에 없다. 클라에서 잘 막아줘야한다.
		P_BATTLEPASS_PURCHASE_Pass(pUser, spBattlePass->m_PassKind, spBattlePass->m_Round, spBattlePass->m_RewardGroupID, BattlePass::ePassType::PAID, resultPoint, passInfo->NeedGem, BattlePass::PURCHASE_PASS_ID, ClientGem, rollbackPoint);
	}
	else
	{
		// 이거는 구매했는지만 검사하고 문제없다면 클라로 was로 다시 요청하라고 보내야함
		NEW_FLATBUFFER(GS_BATTLEPASS_PASS_PURCHASE_ACK, pPACKET);
		pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
		{
			return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_PASS_PURCHASE_ACK(fbb, 0, spBattlePass->m_PassKind, 0, 0, 0, 0, true, passInfo->PackageKind);
		});
		SEND_ACTIVE_USER(pUser, pPACKET);
	}

	return true;
}

bool CBattlePassHelper::P_BATTLEPASS_LOAD(const std::vector<INT64> userIDs, const bool isServerStart)
{
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_LOAD);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			for (const auto& userID : userIDs)
			{
				pQuery->TVP_1.m_nUserID.emplace_back(userID);
			}
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);

			::OnRemoteGAME(ssnid, [=, repo_1 = pQuery->GetSET_1(), repo_2 = pQuery->GetSET_2(), repo_3 = pQuery->GetSET_3(), repo_4 = pQuery->GetSET_4()]()->RECV_RESULT
			{
				// T_BattlePass_Schedule
				for (const auto& row_1 : repo_1)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_1.m_nUserID);
					if (pUser == nullptr)
						continue;

					if (GLOBAL::IsBattleRoyalServer() || GLOBAL::IsServerWarsServer() || CServerInvasionManager::Instance()->IsInvasionUser(pUser->UID()))
						continue;

					if (row_1.m_nState > static_cast<INT16>(BattlePass::ePassState::DAYOFF))
					{
						// 여기서 에러 출력해야할 듯?
						return RECV_OK;
					}
					
					CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(row_1.m_nPassKind);
					if (spBattlePass != nullptr)
					{
						spBattlePass->m_PassKind = row_1.m_nPassKind;
						spBattlePass->m_Round = row_1.m_nRound;
						spBattlePass->m_PassState = static_cast<BattlePass::ePassState>(row_1.m_nState);
						spBattlePass->m_StartTime = std::max(0LL, TLDB::TIME_DB2UTC(row_1.m_tmStartTime));
						spBattlePass->m_EndTime = std::max(0LL, TLDB::TIME_DB2UTC(row_1.m_tmEndTime));
						spBattlePass->m_RewardGroupID = row_1.m_nRewardGroup;

						spBattlePass->m_MissionMap.clear();
						spBattlePass->m_RewardMap.clear();
					}
					else
					{
						spBattlePass = std::make_shared<CBattlePass>();
						if (spBattlePass == nullptr)
							throw std::bad_alloc();

						spBattlePass->m_PassKind = row_1.m_nPassKind;
						spBattlePass->m_Round = row_1.m_nRound;
						spBattlePass->m_PassState = static_cast<BattlePass::ePassState>(row_1.m_nState);
						spBattlePass->m_StartTime = std::max(0LL, TLDB::TIME_DB2UTC(row_1.m_tmStartTime));
						spBattlePass->m_EndTime = std::max(0LL, TLDB::TIME_DB2UTC(row_1.m_tmEndTime));
						spBattlePass->m_RewardGroupID = row_1.m_nRewardGroup;

						spBattlePass->m_MissionMap.clear();
						spBattlePass->m_RewardMap.clear();

						ASE_INSTANCE(pUser, CBattlePassContainer)->Insert(spBattlePass);
					}
				}
				
				// T_BattlePass_Reward
				for (const auto& row_2 : repo_2)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_2.m_nUserID);
					if (pUser == nullptr)
						continue;
					
					CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(row_2.m_nPassKind);
					if (spBattlePass == nullptr)
						continue;

					if(row_2.m_nRound != spBattlePass->m_Round)
						continue;

					if (row_2.m_nRewardType >= static_cast<INT32>(BattlePass::eRewardType::END))
					{
						// 여기서 에러 출력해야할 듯?

						return RECV_OK;
					}
					
					auto rewardMapKey = std::make_pair(row_2.m_nTier, static_cast<BattlePass::eRewardType>(row_2.m_nRewardType));
					
					auto rewardIter = spBattlePass->m_RewardMap.find(rewardMapKey);
					if (spBattlePass->m_RewardMap.end() != rewardIter)
					{
						rewardIter->second->m_PassKind		= row_2.m_nPassKind;
						rewardIter->second->m_Round			= row_2.m_nRound;
						rewardIter->second->m_Tier			= row_2.m_nTier;
						rewardIter->second->m_RewardType	= static_cast<BattlePass::eRewardType>(row_2.m_nRewardType);
					}
					else
					{
						CBattlePassReward::SharedPtr spReward = std::make_shared<CBattlePassReward>();
						if (spReward == nullptr)
							throw std::bad_alloc();

						spReward->m_PassKind	= row_2.m_nPassKind;
						spReward->m_Round		= row_2.m_nRound;
						spReward->m_Tier		= row_2.m_nTier;
						spReward->m_RewardType	= static_cast<BattlePass::eRewardType>(row_2.m_nRewardType);

						spBattlePass->m_RewardMap.emplace(rewardMapKey, spReward);
					}
				}

				// T_BattlePass_Point
				for (const auto& row_3 : repo_3)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_3.m_nUserID);
					if (pUser == nullptr)
						continue;

					CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(row_3.m_nPassKind);
					if (spBattlePass == nullptr)
						continue;

					if (row_3.m_nRound != spBattlePass->m_Round)
						continue;

					if(row_3.m_nPassType >= static_cast<INT32>(BattlePass::ePassType::END) )
					{
						// 여기서 에러 출력해야할 듯?
						return RECV_OK;
					}

					if(IS_NOT_NULL(spBattlePass->m_spPoint))
					{
						spBattlePass->m_spPoint->m_PassKind = row_3.m_nPassKind;
						spBattlePass->m_spPoint->m_Round = row_3.m_nRound;
						spBattlePass->m_spPoint->m_PassType = static_cast<BattlePass::ePassType>(row_3.m_nPassType);
						spBattlePass->m_spPoint->m_curPoint = row_3.m_nPoint;
					}
					else
					{
						spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
						if (spBattlePass->m_spPoint == nullptr)
							throw std::bad_alloc();

						spBattlePass->m_spPoint->m_PassKind = row_3.m_nPassKind;
						spBattlePass->m_spPoint->m_Round = row_3.m_nRound;
						spBattlePass->m_spPoint->m_PassType = static_cast<BattlePass::ePassType>(row_3.m_nPassType);
						spBattlePass->m_spPoint->m_curPoint = row_3.m_nPoint;
					}
				}
				
				// T_BattlePass_Mission
				for (const auto& row_4 : repo_4)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(row_4.m_nUserID);
					if (pUser == nullptr)
						continue;

					CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(row_4.m_nPassKind);
					if (spBattlePass == nullptr)
						continue;

					BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(row_4.m_nPassKind, row_4.m_nMissionKind);
					if (spMissionInfo == nullptr)
						continue;
					
					if (row_4.m_nRound != spBattlePass->m_Round)
						continue;

					if (row_4.m_nState >= static_cast<UINT8>(BattlePass::ePassMissionState::END))
					{
						// 여기서 에러 출력해야할 듯?
						return RECV_OK;
					}
					
					auto missionIter = spBattlePass->m_MissionMap.find(row_4.m_nMissionKind);
					if (spBattlePass->m_MissionMap.end() != missionIter)
					{
						missionIter->second->m_PassKind = row_4.m_nPassKind;
						missionIter->second->m_MissionKind = row_4.m_nMissionKind;
						missionIter->second->m_ConditionKind = spMissionInfo->ConditionKind;
						missionIter->second->m_CurValue = row_4.m_nCurValue;
						missionIter->second->m_Round = row_4.m_nRound;
						missionIter->second->m_MissionState = static_cast<BattlePass::ePassMissionState>(row_4.m_nState);
						missionIter->second->m_RegistTime = std::max(0LL, TLDB::TIME_DB2UTC(row_4.m_tmRegistTime));
					}
					else
					{
						CBattlePassMission::SharedPtr spMission = std::make_shared<CBattlePassMission>();
						if (spMission == nullptr)
							throw std::bad_alloc();

						spMission->m_PassKind = row_4.m_nPassKind;
						spMission->m_MissionKind = row_4.m_nMissionKind;
						spMission->m_ConditionKind = spMissionInfo->ConditionKind;
						spMission->m_CurValue = row_4.m_nCurValue;
						spMission->m_Round = row_4.m_nRound;
						spMission->m_MissionState = static_cast<BattlePass::ePassMissionState>(row_4.m_nState);
						spMission->m_RegistTime = std::max(0LL, TLDB::TIME_DB2UTC(row_4.m_tmRegistTime));
						
						spBattlePass->m_MissionMap.emplace(spMission->m_MissionKind, spMission);
					}

					
				}

				// 서버 기동했을 때 + 복귀 or 서버 이동했을 때
				for (const auto& userID : userIDs)
				{
					CUser* pUser = CUserManager::Instance()->FindByUID(userID);
					if (pUser == nullptr)
						continue;

					ASE_INSTANCE(pUser, CBattlePassContainer)->SetUserID(userID);
					ASE_INSTANCE(pUser, CBattlePassContainer)->SetDirty();
					ASE_INSTANCE(pUser, CBattlePassContainer)->SetUnlockWeekMission();
				}
				
				if (isServerStart)
				{
					if (GameWorkerLoader::STATE::NEXT == BattlePass::CBattlePassManager::Instance()->Recv_DB_BATTLEPASS_LOAD_ACK(nullptr))
						GLOBAL::GAMEWORKER_LOADER.Next();
				}
				else
				{
					if(userIDs.empty() == false)
					{
						// 이거는 휴면복귀했을때
						for(int i = 0; i < userIDs.size(); ++i)
						{
							if (const auto combackUser = CUserManager::Instance()->Seek(userIDs[i]))
								CBattlePassHelper::OnUserLogin(combackUser);
						}
					}
				}
				
				return  RECV_OK;
			});

			END_GDB_QUERY();

			return RECV_OK;
		});

	return true;
}

// 미션 데이터가 변경되었을 때 사용
bool CBattlePassHelper::P_BATTLEPASS_MISSION_UPSERT_TV(CUser* const pUser, CBattlePassMission::UpdateMissionInfos vecMissionUpdate, const BOOL isMissionNewOpen, const BOOL isOnQuestEvent)
{
	if (pUser == nullptr)
		return false;

	// 여기서 캐싱 데이터 변경 x
	CBattlePassMission::RollbackMissionInfos vecMissionRollback;
	
	const INT64 userID = pUser->UID();
	for (const auto& [PassKind, Round, MissionKind, CurValue, State, RegistTime] : vecMissionUpdate)
	{
		CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
		if (spBattlePass == nullptr)
			continue;

		BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(PassKind, MissionKind);
		if (spMissionInfo == nullptr)
			continue;

		if (Round != spBattlePass->m_Round)
			continue;

		auto missionIter = spBattlePass->m_MissionMap.find(MissionKind);
		if (spBattlePass->m_MissionMap.end() != missionIter)
		{
			vecMissionRollback.emplace_back(std::make_tuple(false, missionIter->second->m_PassKind, missionIter->second->m_Round, missionIter->second->m_MissionKind, missionIter->second->m_CurValue, missionIter->second->m_MissionState, missionIter->second->m_RegistTime));
			
			missionIter->second->m_PassKind = PassKind;
			missionIter->second->m_MissionKind = MissionKind;
			missionIter->second->m_ConditionKind = spMissionInfo->ConditionKind;
			missionIter->second->m_CurValue = CurValue;
			missionIter->second->m_Round = Round;
			missionIter->second->m_MissionState = State;
			missionIter->second->m_RegistTime = RegistTime;
		}
		else
		{
			CBattlePassMission::SharedPtr spMission = std::make_shared<CBattlePassMission>();
			if (spMission == nullptr)
				throw std::bad_alloc();


			
			spMission->m_PassKind = PassKind;
			spMission->m_MissionKind = MissionKind;
			spMission->m_ConditionKind = spMissionInfo->ConditionKind;
			spMission->m_CurValue = CurValue;
			spMission->m_Round = Round;
			spMission->m_MissionState = State;
			spMission->m_RegistTime = RegistTime;
			
			spBattlePass->m_MissionMap.emplace(spMission->m_MissionKind, spMission);
			
			vecMissionRollback.emplace_back(std::make_tuple(true, spMission->m_PassKind, spMission->m_Round, spMission->m_MissionKind, spMission->m_CurValue, spMission->m_MissionState, spMission->m_RegistTime));


			if(const auto missionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(PassKind, MissionKind); missionInfo != nullptr)
			{
				if (missionInfo->MissionScheduleType != BattlePass::ePassMissionType::WEEK)
					continue;

				spBattlePass->m_setCheckWeek.emplace(missionInfo->MissionScheduleValue);
			}

		}

	}
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_MISSION_UPSERT_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->TVP_1.clear(); // 미션 데이터
			pQuery->TVP_2.clear(); // 보상으로 지급되는 아이템

			for (const auto& [PassKind, Round, MissionKind, CurValue, State, RegistTime] : vecMissionUpdate)
			{
				pQuery->TVP_1.m_nPassKind.push_back(PassKind);
				pQuery->TVP_1.m_nRound.push_back(Round);
				pQuery->TVP_1.m_nMissionKind.push_back(MissionKind);
				pQuery->TVP_1.m_nCurValue.push_back(CurValue);
				pQuery->TVP_1.m_nState.push_back(static_cast<UINT8>(State));
				pQuery->TVP_1.m_nRegistTime.push_back(TLDB::TIME_UTC2DB(RegistTime));

			}

			// 이 함수=에는 미션 관련 정보만 변경한다.
			pQuery->m_nPassKind = 0;
			pQuery->m_nRound = 0;
			pQuery->m_nBattlePassPoint = 0;
		
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pUser2 = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_MISSION_UPSERT_TV. Fail Result:{}", set_1[0].m_nResult);

						for (const auto& [isNew, PassKind, Round, MissionKind, CurValue, State, RegistTime] : vecMissionRollback)
						{
							if (CBattlePass::SharedPtr rollBackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind); rollBackBattlePass != nullptr)
							{
								auto rollBackMissionIter = rollBackBattlePass->m_MissionMap.find(MissionKind);
								if(rollBackMissionIter != rollBackBattlePass->m_MissionMap.end())
								{
									if (IS_TRUE(isNew))
									{
										rollBackBattlePass->m_MissionMap.erase(rollBackMissionIter);
									}
									else
									{
										rollBackMissionIter->second->m_PassKind = PassKind;
										rollBackMissionIter->second->m_MissionKind = MissionKind;
										rollBackMissionIter->second->m_CurValue = CurValue;
										rollBackMissionIter->second->m_Round = Round;
										rollBackMissionIter->second->m_MissionState = State;
										rollBackMissionIter->second->m_RegistTime = RegistTime;
									}
								}

							}
						}

						return  RECV_OK;
					}

					// 이거는 미션이 처음 등록되었을 때만 사용하는 로직
					for (const auto& [isNew, PassKind, Round, MissionKind, CurValue, State, RegistTime] : vecMissionRollback)
					{
						if (CBattlePass::SharedPtr resultBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind); resultBattlePass != nullptr)
						{
							auto resultMissionIter = resultBattlePass->m_MissionMap.find(MissionKind);

							if (resultMissionIter != resultBattlePass->m_MissionMap.end())
							{
								const auto resultPoint = resultBattlePass->m_spPoint == nullptr ? 0 : resultBattlePass->m_spPoint->m_curPoint;
								const auto resultTier = resultPoint / BattlePass::TIER_TO_POINT;
								const auto resultPassType = resultBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : resultBattlePass->m_spPoint->m_PassType;
								// 미션이 새로 열렸을 때
								if (IS_TRUE(isMissionNewOpen))
									GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_MISSION_OPEN, 0, 0,
										{ PassKind, Round, resultTier, resultPoint , static_cast<INT64>(resultPassType), MissionKind }, {});
								else
								{
									GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_MISSION_PROCESS, 0, 0,
										{ PassKind, Round, resultTier, resultPoint , static_cast<INT64>(resultPassType), MissionKind, CurValue, resultMissionIter->second->m_CurValue, static_cast<INT64>(resultMissionIter->second->m_MissionState) }, {});
								}


							}

						}
					}

					if(IS_TRUE(isOnQuestEvent))
					{
						// 클라로 미션 노티해주자
						NEW_FLATBUFFER(GS_BATTLEPASS_MISSION_NFY, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::BATTLEPASS_MISSION>> vecMissionInfos;

							for (const auto& [PassKind, Round, MissionKind, CurValue, State, RegistTime] : vecMissionUpdate)
							{
								CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
								if (spBattlePass == nullptr)
									continue;

								BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(PassKind, MissionKind);
								if (spMissionInfo == nullptr)
									continue;

								if (Round != spBattlePass->m_Round)
									continue;

								auto missionIter = spBattlePass->m_MissionMap.find(MissionKind);
								if (spBattlePass->m_MissionMap.end() != missionIter)
								{
									vecMissionInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_MISSION(fbb
										, missionIter->second->m_PassKind
										, missionIter->second->m_Round
										, missionIter->second->m_MissionKind
										, missionIter->second->m_CurValue
										, static_cast<INT16>(missionIter->second->m_MissionState)
									));
								}
							}

							return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_MISSION_NFY(fbb, fbb.CreateVector(vecMissionInfos));
						});
						SEND_ACTIVE_USER(pUser, pPACKET);
					}
					
				}
				
				
				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});


	return true;
}


// 미션 완료 보상 받을 때 사용 
bool CBattlePassHelper::P_BATTLEPASS_MISSION_UPSERT_TV_By_MissionComplete(CUser* const pUser, CBattlePassMission::UpdateMissionInfo missionUpdate, CBattlePassPoint::UpdatePassInfo updatePassPoint, CBattlePassMission::UpdateMissionInfo rollBackMission, CBattlePassPoint::UpdatePassInfo rollBackPassPoint)
{
	if (pUser == nullptr)
		return false;

	const INT64 userID = pUser->UID();
	std::vector<BASE::REWARDITEM> vecAddItems;

	const auto& PassKind = std::get<0>(missionUpdate);
	const auto& Round = std::get<1>(missionUpdate);
	const auto& MissionKind = std::get<2>(missionUpdate);
	const auto& CurValue = std::get<3>(missionUpdate);
	const auto& State = std::get<4>(missionUpdate);
	const auto& RegistTime = std::get<5>(missionUpdate);

	CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
	if (spBattlePass == nullptr)
		return false;

	BattlePass::BattlePassMissionInfo::SharedPtr spMissionInfo = BattlePass::CBattlePassManager::Instance()->GetMission(PassKind, MissionKind);
	if (spMissionInfo == nullptr)
		return false;

	if (Round != spBattlePass->m_Round)
		return false;

	auto missionIter = spBattlePass->m_MissionMap.find(MissionKind);
	if (spBattlePass->m_MissionMap.end() != missionIter)
	{
		missionIter->second->m_PassKind = PassKind;
		missionIter->second->m_MissionKind = MissionKind;
		missionIter->second->m_ConditionKind = spMissionInfo->ConditionKind;
		missionIter->second->m_CurValue = CurValue;
		missionIter->second->m_Round = Round;
		missionIter->second->m_MissionState = State;
		missionIter->second->m_RegistTime = RegistTime;
	}
	else
	{
		CBattlePassMission::SharedPtr spMission = std::make_shared<CBattlePassMission>();
		if (spMission == nullptr)
			throw std::bad_alloc();

		spMission->m_PassKind = PassKind;
		spMission->m_MissionKind = MissionKind;
		spMission->m_ConditionKind = spMissionInfo->ConditionKind;
		spMission->m_CurValue = CurValue;
		spMission->m_Round = Round;
		spMission->m_MissionState = State;
		spMission->m_RegistTime = RegistTime;

		spBattlePass->m_MissionMap.emplace(spMission->m_MissionKind, spMission);
	}

	// 배틀 패스 포인트 인멤 갱신
	INT64 prePoint = 0;
	const auto passMaxPoint = static_cast<INT64>(BattlePass::CBattlePassManager::Instance()->GetPassMaxPoint(PassKind, spBattlePass->m_RewardGroupID));
	if (spBattlePass->m_Round == std::get<1>(updatePassPoint))
	{
		if (IS_NOT_NULL(spBattlePass->m_spPoint))
		{
			prePoint = spBattlePass->m_spPoint->m_curPoint;
			spBattlePass->m_spPoint->m_curPoint = std::get<2>(updatePassPoint);
		}
		else
		{
			spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
			if (spBattlePass->m_spPoint == nullptr)
				throw std::bad_alloc();

			spBattlePass->m_spPoint->m_PassKind = std::get<0>(updatePassPoint);
			spBattlePass->m_spPoint->m_Round = std::get<1>(updatePassPoint);
			spBattlePass->m_spPoint->m_PassType = BattlePass::ePassType::FREE;

			prePoint = spBattlePass->m_spPoint->m_curPoint;
			spBattlePass->m_spPoint->m_curPoint = std::get<2>(updatePassPoint);
		}
	}


	// 이미 보상을 획득하기 전에 내 포인트가 맥스포인트를 넘었다면
	if (State == BattlePass::ePassMissionState::REWARDED && prePoint >= static_cast<INT64>(passMaxPoint))
	{
		for (const auto& item : spMissionInfo->vecRewardItems)
		{
			bool bAdded = false;
			for (int k = 0; k < vecAddItems.size(); ++k)
			{
				if (vecAddItems[k].Kind == item.Kind)
				{
					vecAddItems[k].Count += item.Count;
					bAdded = true;
					break;
				}
			}

			if (false == bAdded)
				vecAddItems.push_back(item);

			pUser->GetInventory().AddItemNum(item.Kind, item.Count);
		}
	}
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_MISSION_UPSERT_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->TVP_1.clear(); // 미션 데이터
			pQuery->TVP_2.clear(); // 보상으로 지급되는 아이템

			pQuery->TVP_1.m_nPassKind.push_back(PassKind);
			pQuery->TVP_1.m_nRound.push_back(Round);
			pQuery->TVP_1.m_nMissionKind.push_back(MissionKind);
			pQuery->TVP_1.m_nCurValue.push_back(CurValue);
			pQuery->TVP_1.m_nState.push_back(static_cast<UINT8>(State));
			pQuery->TVP_1.m_nRegistTime.push_back(TLDB::TIME_UTC2DB(RegistTime));

			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}

			pQuery->m_nPassKind = std::get<0>(updatePassPoint);
			pQuery->m_nRound = std::get<1>(updatePassPoint);
			pQuery->m_nBattlePassPoint = std::get<2>(updatePassPoint);

			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()/*, vecMissionRollback*/]()->RECV_RESULT
			{
				if (auto pUser2 = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_MISSION_UPSERT_TV_By_MissionComplete. Fail Result:{}", set_1[0].m_nResult);

						if(CBattlePass::SharedPtr rollbackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind); rollbackBattlePass != nullptr)
						{
							auto rollbackMissionIter = rollbackBattlePass->m_MissionMap.find(MissionKind);
							if (rollbackBattlePass->m_MissionMap.end() != rollbackMissionIter)
							{
								rollbackMissionIter->second->m_PassKind = std::get<0>(rollBackMission);
								rollbackMissionIter->second->m_Round = std::get<1>(rollBackMission);
								rollbackMissionIter->second->m_MissionKind = std::get<2>(rollBackMission);
								rollbackMissionIter->second->m_CurValue = std::get<3>(rollBackMission);
								rollbackMissionIter->second->m_MissionState = std::get<4>(rollBackMission);
								rollbackMissionIter->second->m_RegistTime = std::get<5>(rollBackMission);
							}

							if (IS_NOT_NULL(rollbackBattlePass->m_spPoint))
							{
								rollbackBattlePass->m_spPoint->m_curPoint = std::get<2>(rollBackPassPoint);
							}
						}

						return  RECV_OK;
					}
					// 410011	REASON_BATTLEPASS_MISSION_REWARD	배틀패스 미션 보상 수령	패스kind	회차	이전 티어	티어	포인트	패스 타입	획득 포인트	아이템Kind	아이템Count

					if (const CBattlePass::SharedPtr resultBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind); resultBattlePass != nullptr)
					{
						const auto userCurPoint = resultBattlePass->m_spPoint == nullptr ? 0 : resultBattlePass->m_spPoint->m_curPoint;

						const auto preTier = prePoint / BattlePass::TIER_TO_POINT;
						const auto resultTier = userCurPoint / BattlePass::TIER_TO_POINT;
						auto curType = resultBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : resultBattlePass->m_spPoint->m_PassType;
						
						if (resultTier > preTier)
						{
							GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_TIER_UP, 0, 0,
								{ PassKind, Round, preTier, resultTier, userCurPoint, static_cast<INT64>(curType) }, {});
						}

						for (int i = 0; i < vecAddItems.size(); ++i)
						{
							GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_MISSION_REWARD, 0, 0,
								{ PassKind, Round, preTier, resultTier, userCurPoint, static_cast<INT64>(curType), (userCurPoint - prePoint), vecAddItems[i].Kind, vecAddItems[i].Count }, {});
						}

						GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_MISSION_REWARD_POINT, 0, 0,
							{ PassKind, Round, preTier, resultTier, userCurPoint, static_cast<INT64>(curType), MissionKind, prePoint, (userCurPoint - prePoint) }, {});

						for (int i = 0; i < vecAddItems.size(); ++i)
						{
							GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_MISSION_REWARD_ITEM, 0, 0,
								{ PassKind, Round, preTier, resultTier, userCurPoint, static_cast<INT64>(curType), MissionKind, prePoint, vecAddItems[i].Kind, vecAddItems[i].Count }, {});
						}
						
						NEW_FLATBUFFER(GS_BATTLEPASS_MISSION_COMPLETE_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatRewardItems;

							for (const auto& item : vecAddItems)
								vecFlatRewardItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, item.Count));

							return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_MISSION_COMPLETE_ACK(
								fbb, 0, std::get<0>(updatePassPoint), std::get<2>(updatePassPoint),
								PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_MISSION(fbb, PassKind, Round, MissionKind, CurValue, static_cast<INT16>(State)),
								fbb.CreateVector(vecFlatRewardItems));
						});
						SEND_ACTIVE_USER(pUser, pPACKET);
					}

					
					
				
				}
				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});


	return true;
}

// 구매 했을 때는 멀 구매했는지를 넘겨야할 듯
//  T_BattlePass_Point,  PA_Use_Gem
bool CBattlePassHelper::P_BATTLEPASS_PURCHASE_Tier(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup , const BattlePass::ePassType passType, const INT64 targetPoint, const INT64 useGem, const INT64 purchaseReasonKind, const INT64 prePoint, CBattlePassPoint::RollbackPassInfo rollBackPoint)
{
	if (pUser == nullptr)
		return false;

	const INT64 userID = pUser->UID();

	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
		return false;

	if (IS_NULL(spBattlePass->m_spPoint))
	{
		spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
		if (spBattlePass->m_spPoint == nullptr)
			throw std::bad_alloc();
	}

	spBattlePass->m_spPoint->m_PassType = passType;
	spBattlePass->m_spPoint->m_curPoint = targetPoint;

	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_PURCHASE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nPasskind = passKind;
			pQuery->m_nRound = round;
			pQuery->m_nPassType = static_cast<INT32>(passType);
			pQuery->m_nPoint = targetPoint;
			pQuery->m_nUseGem = useGem;
			pQuery->m_nKind = purchaseReasonKind;
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_PURCHASE_Tier. Fail Result:{}", set_1[0].m_nResult);


						if(const CBattlePass::SharedPtr rollBackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(std::get<0>(rollBackPoint)); rollBackBattlePass!= nullptr)
						{
							if (IS_NOT_NULL(rollBackBattlePass->m_spPoint))
							{
								rollBackBattlePass->m_spPoint->m_PassType = std::get<2>(rollBackPoint);
								rollBackBattlePass->m_spPoint->m_curPoint = std::get<3>(rollBackPoint);
							}
						}

						return  RECV_OK;
					}

					INT32 useVCGem = set_1[0].m_nVC_Gem_Use;
					INT32 useFreeGem = useGem - useVCGem;

					if (useVCGem > 0)	Combine_Manager::Instance()->User_ItemUse(pResultUser,
						{
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_EM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_PE,
							COMBINE::eCOMP_TYPE::EVENT_SPECIAL_MISSION_ITEM_USE
						},
						GAME::VC_GEM_KIND, useVCGem);
					if (useFreeGem > 0)	Combine_Manager::Instance()->User_ItemUse(pResultUser,
						{
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_EM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_PE,
							COMBINE::eCOMP_TYPE::EVENT_SPECIAL_MISSION_ITEM_USE
						},
						GAME::FREE_GEM_KIND, useFreeGem);

					if (const CBattlePass::SharedPtr resultBattlePass = ASE_INSTANCE(pResultUser, CBattlePassContainer)->Seek(passKind); resultBattlePass != nullptr)
					{
						const auto preTier = prePoint / BattlePass::TIER_TO_POINT;
						const auto targetTier = targetPoint / BattlePass::TIER_TO_POINT;
						auto curType = resultBattlePass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : resultBattlePass->m_spPoint->m_PassType;
						const auto curPoint = resultBattlePass->m_spPoint == nullptr ? 0 : resultBattlePass->m_spPoint->m_curPoint;

						GLOBAL::QUEST_MANAGER.OnQuestEvent(pResultUser->UID(), GAME::eEVENTCONDITION_TYPE::USE_GEM_SHOP_SERVER_INVASION, 0, useGem);

						pResultUser->PublicLog_Contents_SendSpendGem(ePublicLogReason::REASON_SPEND_GEM_NEW_BATTLEPASS, useVCGem, useFreeGem);

						GLOBAL::SendLog(pResultUser->UID(), 0LL, DB_LOG::REASON_GS_GEM_USE, 0LL, 0LL, { static_cast<INT64>(GAME::GS_GEM_NEW_BATTLEPASS), pResultUser->GetLanguageType(), 0, 0,
							purchaseReasonKind, useGem, 0, pResultUser->GetAssets().GetGold(), pResultUser->Get_PayingLv(), (set_1[0].m_nGem_After + useGem), useGem, set_1[0].m_nGem_After, 0, 0,
							0, 0, 0, useVCGem, useFreeGem, pResultUser->GetNation(), 0, 0, 0 }, {});

						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_TIER_PURCHASE, 0, 0,
							{ passKind, round, preTier,targetTier ,curPoint, static_cast<INT64>(curType), useVCGem + useFreeGem, set_1[0].m_nGem_After, pResultUser->VipManager().GetVipLevel(), pResultUser->Get_PayingLv() }, {});

						//티어가 이미 10티어 이상일 때 체크해야함!
						GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_TIER_UP, 0, 0,
							{ passKind, round, preTier,targetTier ,curPoint, static_cast<INT64>(curType) }, {});

						if (useVCGem > 0)
							GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::USE_GEM_SERVER_INVASION_RANK_POINT, 0, useVCGem);

						if (useFreeGem > 0)
							GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::USE_GEM_SERVER_INVASION_RANK_POINT, 0, useFreeGem);

						if (IS_ACTIVE_USER(pResultUser))
						{
							// 젬 사용 관련 로직이라 후처리
							NEW_FLATBUFFER(GS_BATTLEPASS_TIER_PURCHASE_ACK, pPACKET);
							pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
							{
								return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_TIER_PURCHASE_ACK(fbb, passKind, round, rewardGroup, targetPoint, set_1[0].m_nGem_After);
							});
							SEND_ACTIVE_USER(pUser, pPACKET);
						}

					}
				}
				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});
	
	return false;
}

// 구매 했을 때는 멀 구매했는지를 넘겨야할 듯
//  T_BattlePass_Point,  PA_Use_Gem
bool CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup, const BattlePass::ePassType passType, const INT64 targetPoint, const INT64 useGem, const INT64 purchaseReasonKind, const INT64 curGem, CBattlePassPoint::RollbackPassInfo rollBackPoint)
{
	if (pUser == nullptr)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG, 
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass User is Null. [PassKind = {}] [Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]",
			passKind, round, rewardGroup, static_cast<INT32>(passType), targetPoint);

		return false;
	}

	const INT64 userID = pUser->UID();

	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass BattlePass Not Exist  [UserID = {}][PassKind =  {}] [Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]",
			pUser->UID(), passKind, round, rewardGroup, static_cast<INT32>(passType), targetPoint);

		return false;
	}

	if (spBattlePass->m_Round != round)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass BattlePass Round Not Match [UserID = {}] [PassKind =  {}] [Server Round = {}] [Request Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]",
			pUser->UID(), passKind, spBattlePass->m_Round, round, rewardGroup, static_cast<INT32>(passType), targetPoint);

		return false;
	}

	if (IS_NULL(spBattlePass->m_spPoint))
	{
		spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
		if (spBattlePass->m_spPoint == nullptr)
			throw std::bad_alloc();
	}

	spBattlePass->m_spPoint->m_PassType = passType;
	spBattlePass->m_spPoint->m_curPoint = targetPoint;
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_PURCHASE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nPasskind = passKind;
			pQuery->m_nRound = round;
			pQuery->m_nPassType = static_cast<INT32>(passType);
			pQuery->m_nPoint = targetPoint;
			pQuery->m_nUseGem = useGem;
			pQuery->m_nKind = purchaseReasonKind;
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
						{
							LOGGER_ERROR(CONST_BATTLEPASS_LOG,
								"P_BATTLEPASS_PURCHASE_Pass. Fail [Result = {}] [ServerID = {}] [UserID = {}] [Request PassKind = {}] [Request Round = {}]  [Request PassType = {}] [Request PassPoint = {}] [DB PassType = {}] [DB PassPoint = {}]",
								set_1[0].m_nResult, GLOBAL::GS_INFO.SVID, userID, passKind, round, static_cast<INT32>(passType), targetPoint, set_1[0].m_nCur_PassType, set_1[0].m_nCur_Point);
						}

						if (const CBattlePass::SharedPtr rollBackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(std::get<0>(rollBackPoint)); rollBackBattlePass != nullptr)
						{
							if (IS_NOT_NULL(rollBackBattlePass->m_spPoint))
							{
								LOGGER_ERROR(CONST_BATTLEPASS_LOG,
									"P_BATTLEPASS_PURCHASE_Pass. Fail RollBack [Result = {}] [ServerID = {}] [UserID = {}] [Requst PassKind = {}] [Request Round = {}] [ PassType {} to {} Rollback ] [PassPoint {} to {} Rollback]",
									set_1[0].m_nResult, GLOBAL::GS_INFO.SVID, userID, passKind, round, static_cast<INT32>(rollBackBattlePass->m_spPoint->m_PassType), static_cast<INT32>(std::get<2>(rollBackPoint)), rollBackBattlePass->m_spPoint->m_curPoint, std::get<3>(rollBackPoint));
								
								rollBackBattlePass->m_spPoint->m_PassType = std::get<2>(rollBackPoint);
								rollBackBattlePass->m_spPoint->m_curPoint = std::get<3>(rollBackPoint);
							}
						}

						// 젬 수량이 틀어진 경우다. fail하면 set_1[0].m_nGem_After 에 db 기준 현재 젬 수량이 담겨 있다.
						if(set_1[0].m_nResult == -20)
						{
							if (IS_ACTIVE_USER(pResultUser))
							{
								// 젬 사용 관련 로직이라 후처리
								NEW_FLATBUFFER(GS_BATTLEPASS_PASS_PURCHASE_ACK, pPACKET);
								pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
								{
									return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_PASS_PURCHASE_ACK(fbb, RESULT::R_FAIL_DISCORD_ASSET, passKind, round, static_cast<INT32>(passType), targetPoint, pQuery->SET_1.m_nGem_After, false, 0);
								});
								SEND_ACTIVE_USER(pResultUser, pPACKET);
							}
						}

						return  RECV_OK;
					}

					INT32 useVCGem = set_1[0].m_nVC_Gem_Use;
					INT32 useFreeGem = useGem - useVCGem;

					if (useVCGem > 0)	Combine_Manager::Instance()->User_ItemUse(pResultUser,
						{
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_EM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_PE,
							COMBINE::eCOMP_TYPE::EVENT_SPECIAL_MISSION_ITEM_USE
						},
						GAME::VC_GEM_KIND, useVCGem);
					if (useFreeGem > 0)	Combine_Manager::Instance()->User_ItemUse(pResultUser,
						{
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_EM,
							COMBINE::eCOMP_TYPE::EVENT_USE_ITEM_TO_PE,
							COMBINE::eCOMP_TYPE::EVENT_SPECIAL_MISSION_ITEM_USE
						},
						GAME::FREE_GEM_KIND, useFreeGem);

					if (const CBattlePass::SharedPtr resultBattlePass = ASE_INSTANCE(pResultUser, CBattlePassContainer)->Seek(passKind); resultBattlePass != nullptr && resultBattlePass->m_PassState == BattlePass::ePassState::PROCESSING)
					{
						auto curTier = resultBattlePass->m_spPoint->m_curPoint / BattlePass::TIER_TO_POINT;
						auto curType = resultBattlePass->m_spPoint->m_PassType;

						GLOBAL::QUEST_MANAGER.OnQuestEvent(pResultUser->UID(), GAME::eEVENTCONDITION_TYPE::USE_GEM_SHOP_SERVER_INVASION, 0, useGem);

						pResultUser->PublicLog_Contents_SendSpendGem(ePublicLogReason::REASON_SPEND_GEM_NEW_BATTLEPASS, useVCGem, useFreeGem);

						GLOBAL::SendLog(pResultUser->UID(), 0LL, DB_LOG::REASON_GS_GEM_USE, 0LL, 0LL, { static_cast<INT64>(GAME::GS_GEM_NEW_BATTLEPASS), pResultUser->GetLanguageType(), 0, 0,
							purchaseReasonKind, useGem, 0, pResultUser->GetAssets().GetGold(), pResultUser->Get_PayingLv(), (set_1[0].m_nGem_After + useGem), useGem, set_1[0].m_nGem_After, 0, 0,
							0, 0, 0, useVCGem, useFreeGem, pResultUser->GetNation(), 0, 0, 0 }, {});

						GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE, 0, 0,
							{ passKind , round, curTier, targetPoint, static_cast<INT64>(curType), useVCGem, set_1[0].m_nGem_After, pResultUser->VipManager().GetVipLevel(), pResultUser->Get_PayingLv(),
							set_1[0].m_nVC_Gem_Use,  set_1[0].m_nVC_Gem_After,  (useGem - set_1[0].m_nVC_Gem_Use) , (set_1[0].m_nGem_After - set_1[0].m_nVC_Gem_After) },
							{});

						if (useVCGem > 0)
							GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::USE_GEM_SERVER_INVASION_RANK_POINT, 0, useVCGem);

						if (useFreeGem > 0)
							GLOBAL::QUEST_MANAGER.OnQuestEvent(userID, GAME::eEVENTCONDITION_TYPE::USE_GEM_SERVER_INVASION_RANK_POINT, 0, useFreeGem);

						// 사용후 전체젬  set_1[0].m_nGem_After
						// 사용한 전체젬  useGem
						// 사용한 유가젬  set_1[0].m_nVC_Gem_Use
						// 사용한 무가젬  useGem - set_1[0].m_nVC_Gem_Use
						
						if (const auto passScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(passKind); passScheduleInfo != nullptr)
						{
							// 패스 구매 완료 후 아이템 지급
							std::vector<PROTOCOL::ITEM_INFO> vecPresent;
							if (const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(passKind); passInfo != nullptr)
							{
								const auto& vecSeasonalBuff = passInfo->vecSeasonalBuff;
								for (int i = 0; i < vecSeasonalBuff.size(); ++i)
								{
									int iBuffKind = vecSeasonalBuff[i];

									GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE_BUFF, 0, 0,
										{ passKind, round, curTier, targetPoint,static_cast<INT64>(curType), iBuffKind,
										  set_1[0].m_nVC_Gem_Use,  set_1[0].m_nVC_Gem_After,  (useGem - set_1[0].m_nVC_Gem_Use) , (set_1[0].m_nGem_After - set_1[0].m_nVC_Gem_After) }, {});
									SetBuffItem(pResultUser->UID(), passKind, iBuffKind);
								}

								auto& inventory = pResultUser->GetInventory();

								const auto& vecSpecialBenefit = passInfo->vecSpecialBenefit;
								for (int i = 0; i < vecSpecialBenefit.size(); ++i)
								{
									const INT32 iKind = vecSpecialBenefit[i].Kind;
									const INT64 iCount = vecSpecialBenefit[i].Count;
									const INT32 curCount = inventory.GetItemNum(iKind);

									pResultUser->GiveItems(pResultUser->UID(), iKind, iCount, curCount, DB_LOG::REASON_BATTLEPASS_PASS_PIECE_ITEM_GET);
								}

								const auto& vecGuildPresent = passInfo->vecGuildPresent;
								for (int i = 0; i < vecGuildPresent.size(); ++i)
								{
									vecPresent.push_back(PROTOCOL::ITEM_INFO(vecGuildPresent[i].Kind, vecGuildPresent[i].Count));
								}

								if (false == vecPresent.empty())
								{
									CGuild* pGuild = CGuildManager::Instance()->GetGuild(pResultUser->GetGuildID());
									if (IS_NOT_NULL(pGuild))
									{
										auto vecID = pGuild->GetMemberUIDs();
										for (int i = 0; i < vecID.size(); ++i)
										{
											CUser* pGuildMember = CUserManager::Instance()->FindByUID(vecID[i]);
											if (IS_NULL(pGuildMember))
											{
												continue;
											}

											INT16 languageType = pGuildMember->GetLanguageType();

											std::wstring passMailTitleTextKey = _T("");
											std::wstring passMailContentTextKey = _T("");

											if (passInfo->NeedGem > 0)
											{
												passMailTitleTextKey = _T("MAIL_SEASON_PASS_GET_TITLE");
												passMailContentTextKey = _T("MAIL_SEASON_PASS_GET_TEXT");
											}
											else
											{
												passMailTitleTextKey = _T("UI_PRIME_PASS{0}_TEXT_30");
												passMailContentTextKey = _T("UI_PRIME_PASS{0}_TEXT_31");
												UTIL::ReplaceAll(passMailTitleTextKey, _T("{0}"), UTIL::StringFormat(L"%02lld", passKind));
												UTIL::ReplaceAll(passMailContentTextKey, _T("{0}"), UTIL::StringFormat(L"%02lld", passKind));
											}

											std::wstring title = GetText(languageType, passMailTitleTextKey.c_str());
											UTIL::ReplaceAll(title, _T("{0}"), pResultUser->m_lord.GetName());

											std::wstring contents = GetText(languageType, passMailContentTextKey.c_str());
											UTIL::ReplaceAll(contents, _T("{0}"), pResultUser->m_lord.GetName());
											UTIL::ReplaceAll(contents, _T("{1}"), pGuild->GetGuildNickName());

											CMailManager::Instance()->SendAppendingMail(pGuildMember->UID(), _T(""), _T(""), title, contents, vecPresent, GAME::eMAIL_TYPE::MAIL_TYPE_SEASONPASS_GUILD_PRESENT);

											GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE_GUILD_MAIL, 0, 0,
												{ passKind, round, curTier, targetPoint, static_cast<INT64>(curType), pResultUser->GetGuildID(), vecID[i], }, {});
										}
									}
								}

							}
						}


					}
					
					if (IS_ACTIVE_USER(pResultUser))
					{
						// 젬 사용 관련 로직이라 후처리
						NEW_FLATBUFFER(GS_BATTLEPASS_PASS_PURCHASE_ACK, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_PASS_PURCHASE_ACK(fbb, 0, passKind, round, static_cast<INT32>(passType), targetPoint, pQuery->SET_1.m_nGem_After, false, 0);
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}
					
				}
				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});

	return false;
}

bool CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass_By_WAS(CUser* const pUser, const INT32 passKind, const INT32 round, const INT32 rewardGroup, const BattlePass::ePassType passType, const INT64 targetPoint, CBattlePassPoint::RollbackPassInfo rollBackPoint)
{
	if (pUser == nullptr)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG, 
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass_By_WAS User is Null. [PassKind = {}] [Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]", 
			passKind, round, rewardGroup, static_cast<INT32>(passType), targetPoint);
		
		return false;
	}

	const INT64 userID = pUser->UID();

	// 여기는 was에서 구매 완료 처리를 해준 상태라 먼저 인멤 갱신해라
	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass_By_WAS BattlePass Not Exist  [UserID = {}][PassKind =  {}] [Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]", 
			pUser->UID(), passKind, round, rewardGroup, static_cast<INT32>(passType), targetPoint);

		return false;
	}

	if (spBattlePass->m_Round != round)
	{
		LOGGER_ERROR(CONST_BATTLEPASS_LOG,
			"CBattlePassHelper::P_BATTLEPASS_PURCHASE_Pass_By_WAS BattlePass Round Not Match [UserID = {}] [PassKind =  {}] [Server Round = {}] [Request Round = {}] [RewardGroup = {}] [PassType = {}] [TargetPoint = {}]", 
			pUser->UID(), passKind, spBattlePass->m_Round, round, rewardGroup, static_cast<INT32>(passType), targetPoint);

		return false;
	}
	
	if (IS_NULL(spBattlePass->m_spPoint))
	{
		spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
		if (spBattlePass->m_spPoint == nullptr)
			throw std::bad_alloc();
	}

	spBattlePass->m_spPoint->m_PassType = passType;
	spBattlePass->m_spPoint->m_curPoint = targetPoint;
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_PURCHASE);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nPasskind = passKind;
			pQuery->m_nRound = round;
			pQuery->m_nPassType = static_cast<INT32>(passType);
			pQuery->m_nPoint = targetPoint;
			pQuery->m_nUseGem = 0;
			pQuery->m_nKind = 0;
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()]()->RECV_RESULT
			{
				if (const auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{	
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
						{
							LOGGER_ERROR(CONST_BATTLEPASS_LOG, 
								"P_BATTLEPASS_PURCHASE_Pass_By_WAS. Fail [Result = {}] [ServerID = {}] [UserID = {}] [Request PassKind = {}] [Request Round = {}]  [Request PassType = {}] [Request PassPoint = {}] [DB PassType = {}] [DB PassPoint = {}]", 
								set_1[0].m_nResult, GLOBAL::GS_INFO.SVID, userID, passKind, round, static_cast<INT32>(passType), targetPoint, set_1[0].m_nCur_PassType, set_1[0].m_nCur_Point);
						}


						if (const CBattlePass::SharedPtr rollBackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(std::get<0>(rollBackPoint)); rollBackBattlePass != nullptr)
						{
							if (IS_NOT_NULL(rollBackBattlePass->m_spPoint))
							{
								LOGGER_ERROR(CONST_BATTLEPASS_LOG,
									"P_BATTLEPASS_PURCHASE_Pass_By_WAS. Fail RollBack [Result = {}] [ServerID = {}] [UserID = {}] [Requst PassKind = {}] [Request Round = {}] [ PassType {} to {} Rollback ] [PassPoint {} to {} Rollback]",
									set_1[0].m_nResult, GLOBAL::GS_INFO.SVID, userID, passKind, round, static_cast<INT32>(rollBackBattlePass->m_spPoint->m_PassType), static_cast<INT32>(std::get<2>(rollBackPoint)), rollBackBattlePass->m_spPoint->m_curPoint, std::get<3>(rollBackPoint));
								
								rollBackBattlePass->m_spPoint->m_PassType = std::get<2>(rollBackPoint);
								rollBackBattlePass->m_spPoint->m_curPoint = std::get<3>(rollBackPoint);
							}
						}

						return  RECV_OK;
					}


					if (const CBattlePass::SharedPtr resultBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind); resultBattlePass != nullptr && resultBattlePass->m_PassState == BattlePass::ePassState::PROCESSING)
					{
						//구매로그
						auto curTier = resultBattlePass->m_spPoint->m_curPoint / BattlePass::TIER_TO_POINT;
						auto curType = resultBattlePass->m_spPoint->m_PassType;
						GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE, 0, 0,
							{ passKind , round, curTier, targetPoint, static_cast<INT64>(curType), 0, 0, pUser->VipManager().GetVipLevel(), pUser->Get_PayingLv(),
							set_1[0].m_nVC_Gem_Use,  set_1[0].m_nVC_Gem_After,  0 , (set_1[0].m_nGem_After - set_1[0].m_nVC_Gem_After) },
							{});

						if (const auto passScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(passKind); passScheduleInfo != nullptr)
						{
							// 패스 구매 완료 후 아이템 지급
							std::vector<PROTOCOL::ITEM_INFO> vecPresent;

							if (const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(passKind); passInfo != nullptr)
							{
								const auto& vecSeasonalBuff = passInfo->vecSeasonalBuff;
								for (int i = 0; i < vecSeasonalBuff.size(); ++i)
								{
									int iBuffKind = vecSeasonalBuff[i];

									GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE_BUFF, 0, 0,
										{ passKind, round, curTier, targetPoint,static_cast<INT64>(curType), iBuffKind,
										set_1[0].m_nVC_Gem_Use,  set_1[0].m_nVC_Gem_After,  0 , (set_1[0].m_nGem_After - set_1[0].m_nVC_Gem_After) }, {});
									SetBuffItem(pResultUser->UID(), passKind, iBuffKind);
								}

								auto& inventory = pResultUser->GetInventory();

								const auto& vecSpecialBenefit = passInfo->vecSpecialBenefit;
								for (int i = 0; i < vecSpecialBenefit.size(); ++i)
								{
									const INT32 iKind = vecSpecialBenefit[i].Kind;
									const INT64 iCount = vecSpecialBenefit[i].Count;
									const INT32 curCount = inventory.GetItemNum(iKind);

									// 이거... 리즌 추가 필요하다...
									pResultUser->GiveItems(pResultUser->UID(), iKind, iCount, curCount, DB_LOG::REASON_SEASONPASS_PASS_PURCHASE_GIVE_ITEM);
								}

								const auto& vecGuildPresent = passInfo->vecGuildPresent;
								for (int i = 0; i < vecGuildPresent.size(); ++i)
								{
									vecPresent.push_back(PROTOCOL::ITEM_INFO(vecGuildPresent[i].Kind, vecGuildPresent[i].Count));
								}

								if (false == vecPresent.empty())
								{
									CGuild* pGuild = CGuildManager::Instance()->GetGuild(pUser->GetGuildID());
									if (IS_NOT_NULL(pGuild))
									{
										auto vecID = pGuild->GetMemberUIDs();
										for (int i = 0; i < vecID.size(); ++i)
										{
											CUser* pGuildMember = CUserManager::Instance()->FindByUID(vecID[i]);
											if (IS_NULL(pGuildMember))
											{
												continue;
											}

											INT16 languageType = pGuildMember->GetLanguageType();
											std::wstring passMailTitleTextKey = _T("");
											std::wstring passMailContentTextKey = _T("");

											// 배틀 패스라면 (= 젬으로 구매하는 페스라면)
											if (passInfo->NeedGem > 0)
											{
												passMailTitleTextKey = _T("MAIL_SEASON_PASS_GET_TITLE");
												passMailContentTextKey = _T("MAIL_SEASON_PASS_GET_TEXT");
											}
											else
											{
												passMailTitleTextKey = _T("UI_PRIME_PASS{0}_TEXT_30");
												passMailContentTextKey = _T("UI_PRIME_PASS{0}_TEXT_31");
												UTIL::ReplaceAll(passMailTitleTextKey, _T("{0}"), UTIL::StringFormat(L"%02lld", passKind));
												UTIL::ReplaceAll(passMailContentTextKey, _T("{0}"), UTIL::StringFormat(L"%02lld", passKind));
											}


											std::wstring title = GetText(languageType, passMailTitleTextKey.c_str());
											UTIL::ReplaceAll(title, _T("{0}"), pResultUser->m_lord.GetName());

											std::wstring contents = GetText(languageType, passMailContentTextKey.c_str());
											UTIL::ReplaceAll(contents, _T("{0}"), pResultUser->m_lord.GetName());
											UTIL::ReplaceAll(contents, _T("{1}"), pGuild->GetGuildNickName());

											CMailManager::Instance()->SendAppendingMail(pGuildMember->UID(), _T(""), _T(""), title, contents, vecPresent, GAME::eMAIL_TYPE::MAIL_TYPE_SEASONPASS_GUILD_PRESENT);

											GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_PASS_PURCHASE_GUILD_MAIL, 0, 0,
												{ passKind, round, curTier, targetPoint, static_cast<INT64>(curType), pResultUser->GetGuildID(), vecID[i], }, {});
										}
									}
								}

							}
						}


					}

					if (IS_ACTIVE_USER(pResultUser))
					{
						NEW_FLATBUFFER(GS_BATTLEPASS_PASS_PURCHASE_NFY, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_PASS_PURCHASE_NFY(fbb, 0, passKind, round, static_cast<INT32>(passType), targetPoint);
						});
						SEND_ACTIVE_USER(pUser, pPACKET);
					}
				}
				
				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});

	return false;
}


// _BattlePass_Reward,  TV_BattlePass_Reward
// 티어별 보상 받는 곳
bool CBattlePassHelper::P_BATTLEPASS_REWARD_SET_TV(CUser* const pUser, const INT32 passKind, CBattlePassReward::UpdateRewardInfos vecPassRewardUpdate/*, CBattlePassReward::UpdateRewardInfos vecPassRewardRollback*/)
{
	if (pUser == nullptr)
		return false;

	const INT64 userID = pUser->UID();

	INT64 AddOil = 0;
	INT64 AddIron = 0;
	INT64 AddSilver = 0;
	INT64 AddGold = 0;
	INT64 AddGem = 0;
	INT64 AddFreeGem = 0;

	std::vector<BASE::REWARDITEM> vecAddItems;
	std::vector<BASE::COUPON>	vecCoupons;
	
	// 여기에서는 로직 다 돌아서 지급할 수 있는 보상들이 들어갔음
	for (const auto& [PassKind, Round, Tier, RewardType] : vecPassRewardUpdate)
	{
		CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
		if (spBattlePass == nullptr)
			continue;

		BattlePass::BattlePassPointinfo::SharedPtr spRewardInfo = BattlePass::CBattlePassManager::Instance()->GetPointInfo(PassKind, spBattlePass->m_RewardGroupID, Tier);
		if (spRewardInfo == nullptr)
			continue;

		if (Round != spBattlePass->m_Round)
			continue;

		const auto rewardKey = make_pair(Tier, RewardType);
		
		auto rewardIter = spBattlePass->m_RewardMap.find(rewardKey);
		if (spBattlePass->m_RewardMap.end() != rewardIter)
			continue;

		CBattlePassReward::SharedPtr spReward = std::make_shared<CBattlePassReward>();
		if (spReward == nullptr)
			throw std::bad_alloc();

		spReward->m_PassKind = PassKind;
		spReward->m_Round = Round;
		spReward->m_Tier = Tier;
		spReward->m_RewardType = RewardType;

		spBattlePass->m_RewardMap.emplace(rewardKey, spReward);

		// 인멤 로직 처리
		std::vector<BASE::REWARDITEM> vecRewarditems;
		switch(RewardType)
		{
		case BattlePass::eRewardType::FREE: vecRewarditems = spRewardInfo->vecFreeReward; break;
		case BattlePass::eRewardType::PAID: vecRewarditems = spRewardInfo->vecPaidReward; break;
		default:
			continue;
		}

		for (const auto& reward : vecRewarditems)
		{
			auto iteminfo = BASE::GET_ITEM_DATA(reward.Kind);
			if (iteminfo == nullptr) continue;

			if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
			{
				if (reward.Kind == 1)		AddOil += reward.Count;
				else if (reward.Kind == 2)	AddIron += reward.Count;
				else if (reward.Kind == 3)	AddSilver += reward.Count;
				else if (reward.Kind == 4)	AddGold += reward.Count;
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
					endTime = GetDueDay_UTC(SECOND_PER_DAY * 30LL * 12LL * 100LL);	// 100년 뒤
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
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_REWARD_SET_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->TVP_1.clear(); // 보상kind

			// INT64, INT32, INT32, INT32, BattlePass::eRewardType
			for (const auto& [PassKind, Round, Tier, RewardType] : vecPassRewardUpdate)
			{
				pQuery->TVP_1.m_nPassKind.push_back(PassKind);
				pQuery->TVP_1.m_nRound.push_back(Round);
				pQuery->TVP_1.m_nTier.push_back(Tier);
				pQuery->TVP_1.m_nRewardType.push_back(static_cast<INT32>(RewardType));
			}

			// 보상 아이템
			pQuery->TVP_2.clear();
			for (const auto& item : vecAddItems)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}
		
			// 아이템 쿠폰은 교환 이벤트 보고 채우자
			// 보상 쿠폰
			pQuery->TVP_3.clear();
			for (const auto& coupon : vecCoupons)
			{
				pQuery->TVP_3.m_nItemKind.push_back(coupon.ItemKind);
				pQuery->TVP_3.m_nUniqueID.push_back(coupon.UniqueID);
				pQuery->TVP_3.m_nisActive.push_back(coupon.isActive);
				pQuery->TVP_3.m_nBeginTime.push_back(TLDB::TIME_UTC2DB(coupon.BeginTime));
				pQuery->TVP_3.m_nEndTime.push_back(TLDB::TIME_UTC2DB(coupon.EndTime));
				pQuery->TVP_3.m_nDiscountRate.push_back(coupon.DiscountRate);

				NFixStringW<GAME::COUPON_PRODUCT_KIND_MAX_LEN_NUL> strProductKinds((LPCTSTR)coupon.ProductKind.c_str());
				pQuery->TVP_3.m_nProductKinds.push_back(strProductKinds);
				pQuery->TVP_3.m_nProductKindsLen.push_back(static_cast<INT64>(strProductKinds.GetLength()) * static_cast<INT64>(strProductKinds.GetCharTypeSize()));
			}
		
			// 자원
			pQuery->m_nAddOil = AddOil;
			pQuery->m_nAddIron = AddIron;
			pQuery->m_nAddSilver = AddSilver;
			pQuery->m_nAddGold = AddGold;

			pQuery->m_nAddGem = AddGem;
			pQuery->m_nAddFreeGem = AddFreeGem;
		
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1(), set_2 = pQuery->GetSET_2()/*, vecPassRewardRollback*/]()->RECV_RESULT
			{
				if (auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_REWARD_SET_TV. Fail Result:{}", set_1[0].m_nResult);

						for (const auto& [PassKind, Round, Tier, RewardType] : vecPassRewardUpdate)
						{
							CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
							if (spBattlePass == nullptr)
								continue;

							BattlePass::BattlePassPointinfo::SharedPtr spRewardInfo = BattlePass::CBattlePassManager::Instance()->GetPointInfo(PassKind, spBattlePass->m_RewardGroupID, Tier);
							if (spRewardInfo == nullptr)
								continue;

							if (Round != spBattlePass->m_Round)
								continue;

							const auto rewardKey = make_pair(Tier, RewardType);

							auto rewardIter = spBattlePass->m_RewardMap.find(rewardKey);
							if (spBattlePass->m_RewardMap.end() != rewardIter)
								spBattlePass->m_RewardMap.erase(rewardIter);
						}

						return  RECV_OK;
					}

					if (const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind); spBattlePass != nullptr)
					{
						
						//로그를 남기려면...
						//const auto curTier = spBattlePass->m_spPoint->m_curPoint / BattlePass::TIER_TO_POINT;
						const auto curPoint = spBattlePass->m_spPoint->m_curPoint;
						const auto curType = spBattlePass->m_spPoint->m_PassType;
					

						// 자원 획득 후처리
						PROTOCOL::ASSET addAsset;
						if (AddOil > 0)		addAsset.AddOil(AddOil);
						if (AddIron > 0)	addAsset.AddIron(AddIron);
						if (AddSilver > 0)	addAsset.AddSilver(AddSilver);
						if (AddGold > 0)	addAsset.AddGold(AddGold);

						pResultUser->GetAssets().AddAssets(addAsset);

						// 신규 로그 추가가 필요하다!
						if (false == addAsset.IsEmpty())
						{
							//GLOBAL::SendLogResourceGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_RESOURCE, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(),
							//	{ passKind, rewardID, }, {});
						}

						// 쿠폰 획득 로그
						for (const auto& coupon : set_2)
						{
							auto couponBeginTime = coupon.m_tmBeginTime;
							auto couponEndTime	 = coupon.m_tmEndTime;
							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_COUPON_GET, 0, 0, { coupon.m_nUniqueID, coupon.m_nItemKind, 0, 1, 0,
								static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)), static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)) }, {});
						}

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

						if (0 < AddFreeGem || 0 < AddGem)
						{
							pUser->PublicLog_Contents_SendAddGem(ePublicLogReason::REASON_ADD_GEM_BATTLEPASS_REWARD, 0, AddGem, AddFreeGem);
						}

						for (const auto& [PassKind, Round, Tier, RewardType] : vecPassRewardUpdate)
						{
							BattlePass::BattlePassPointinfo::SharedPtr pointInfo = BattlePass::CBattlePassManager::Instance()->GetPointInfo(PassKind, spBattlePass->m_RewardGroupID, Tier);
							if (pointInfo == nullptr)
								continue;

							//티어 획득 로그
							GLOBAL::SendLog(pResultUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_TIER_REWARD, 0, 0,
								{ PassKind, spBattlePass->m_RewardGroupID,Round, Tier, curPoint, static_cast<INT64>(curType), static_cast<INT64>(RewardType) }, {});

							if (RewardType == BattlePass::eRewardType::FREE)
							{
								for (int i = 0; i < pointInfo->vecFreeReward.size(); ++i)
								{
									auto iteminfo = BASE::GET_ITEM_DATA(pointInfo->vecFreeReward[i].Kind);
									if (iteminfo == nullptr)
										continue;

									if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
									{
										INT64 logOil = 0;
										INT64 logIron = 0;
										INT64 logSilver = 0;
										INT64 logGold = 0;

										if (pointInfo->vecFreeReward[i].Kind == 1)		logOil += pointInfo->vecFreeReward[i].Count;
										else if (pointInfo->vecFreeReward[i].Kind == 2)	logIron += pointInfo->vecFreeReward[i].Count;
										else if (pointInfo->vecFreeReward[i].Kind == 3)	logSilver += pointInfo->vecFreeReward[i].Count;
										else if (pointInfo->vecFreeReward[i].Kind == 4)	logGold += pointInfo->vecFreeReward[i].Count;

										//자원 획득 로그
										if (logOil > 0 || logIron > 0 || logSilver > 0)
										{
											GLOBAL::SendLogResourceGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_RESOURCE, logOil, logIron, logSilver,
												{ passKind, spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, {});
										}
										//골드 획득 로그
										if (logGold > 0)
										{
											GLOBAL::SendLogGoldGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_GOLD, logGold,
												{ passKind, spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, { });
										}
									}
									else if (iteminfo->i32ITEM_TYPE != GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE && iteminfo->i32ITEM_TYPE != GAME::eITEM_TYPE::ITEM_TYPE_COUPON)
									{
										//일반 아이템 획득 로그
										GLOBAL::SendLogItemGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_ITEM, pointInfo->vecFreeReward[i].Kind, pointInfo->vecFreeReward[i].Count,
											{ passKind,spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, {});
									}
								}
							}
							else if (RewardType == BattlePass::eRewardType::PAID)
							{
								for (int i = 0; i < pointInfo->vecPaidReward.size(); ++i)
								{
									auto iteminfo = BASE::GET_ITEM_DATA(pointInfo->vecPaidReward[i].Kind);
									if (iteminfo == nullptr)
										continue;

									if (iteminfo->i32ITEM_TYPE == GAME::eITEM_TYPE::ITEM_NATIVE_RESOURCE)		// 자원
									{
										INT64 logOil = 0;
										INT64 logIron = 0;
										INT64 logSilver = 0;
										INT64 logGold = 0;

										if (pointInfo->vecPaidReward[i].Kind == 1)		logOil += pointInfo->vecPaidReward[i].Count;
										else if (pointInfo->vecPaidReward[i].Kind == 2)	logIron += pointInfo->vecPaidReward[i].Count;
										else if (pointInfo->vecPaidReward[i].Kind == 3)	logSilver += pointInfo->vecPaidReward[i].Count;
										else if (pointInfo->vecPaidReward[i].Kind == 4)	logGold += pointInfo->vecPaidReward[i].Count;

										//자원 획득 로그
										if (logOil > 0 || logIron > 0 || logSilver > 0)
										{
											GLOBAL::SendLogResourceGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_RESOURCE, logOil, logIron, logSilver,
												{ passKind, spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, {});
										}
										//골드 획득 로그
										if (logGold > 0)
										{
											GLOBAL::SendLogGoldGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_GOLD, logGold,
												{ passKind, spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, { });
										}
									}
									else if (iteminfo->i32ITEM_TYPE != GAME::eITEM_TYPE::ITEM_TYPE_GEMSTONE && iteminfo->i32ITEM_TYPE != GAME::eITEM_TYPE::ITEM_TYPE_COUPON)
									{
										//일반 아이템 획득 로그
										GLOBAL::SendLogItemGet(pResultUser->UID(), DB_LOG::REASON_BATTLEPASS_TIER_REWARD_ITEM, pointInfo->vecPaidReward[i].Kind, pointInfo->vecPaidReward[i].Count,
											{ passKind,spBattlePass->m_RewardGroupID, Round, Tier, curPoint, static_cast<INT64>(RewardType), }, {});
									}
								}
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
							NEW_FLATBUFFER(GS_BATTLEPASS_TIER_REWARD_ACK, pPACKET);
							pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
							{
								std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::INFO_ITEM>> vecFlatAddItems;
								for (const auto& item : vecAddItems)
									vecFlatAddItems.emplace_back(PROTOCOL::FLATBUFFERS::CreateINFO_ITEM(fbb, item.Kind, item.Count));

								std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::COUPON>>	vecFlatCoupons;
								for (const auto& coupon : set_2)
								{
									auto couponBeginTime = coupon.m_tmBeginTime;
									auto couponEndTime = coupon.m_tmEndTime;
									vecFlatCoupons.emplace_back(PROTOCOL::FLATBUFFERS::CreateCOUPON(fbb, coupon.m_nUniqueID, coupon.m_nisActive, coupon.m_nItemKind,
										static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponBeginTime)),
										static_cast<int64_t>(GLOBAL::TIME_DB2UTC(couponEndTime)), coupon.m_nDiscountRate,
										::to_flatbuffer(fbb, coupon.m_szProductKinds)));
								}

								auto sendAddAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, addAsset.GetOil(), addAsset.GetIron(), addAsset.GetSilver(), addAsset.GetGold());
								auto curAsset = PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pResultUser->GetAssets().GetOil(), pResultUser->GetAssets().GetIron(), pResultUser->GetAssets().GetSilver(), pResultUser->GetAssets().GetGold());

								std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::BATTLEPASS_REWARD>>  vecRewardInfos;
								for (const auto& [PassKind, Round, Tier, RewardType] : vecPassRewardUpdate)
								{
									vecRewardInfos.emplace_back(PROTOCOL::FLATBUFFERS::CreateBATTLEPASS_REWARD(fbb
										, PassKind
										, Round
										, Tier
										, static_cast<INT16>(RewardType)
									));
								}

								return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_TIER_REWARD_ACK(fbb, 0, passKind, fbb.CreateVector(vecRewardInfos), fbb.CreateVector(vecFlatAddItems), sendAddAsset, curAsset, AddGem, AddFreeGem, fbb.CreateVector(vecFlatCoupons));
							});
							SEND_ACTIVE_USER(pResultUser, pPACKET);
						}


						if (mapDirectUse.size() > 0)
						{
							for (const auto& itemDU : mapDirectUse)
							{
								const auto itemCountCurrent = pResultUser->GetInventory().GetItemNum(itemDU.first);
								ItemUse::UseProcess(pResultUser, itemDU.first, itemDU.second, itemCountCurrent);
							}
						}
					}
				}

				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});
	
	return false;
}

// T_BattlePass_Schedule 
bool CBattlePassHelper::P_BATTLEPASS_SCHEDULE_UPSERT_TV(CUser* const pUser, const INT64 curTime, CBattlePass::UpdateScheduleInfos vecScheduleUpdate, std::vector<BASE::REWARDITEM> vecSetItem, std::vector<BASE::REWARDITEM> vecRollBackItem)
{
	if (pUser == nullptr)
		return false;

	const INT64 userID = pUser->UID();

	CBattlePass::RollbackScheduleInfos vecRollbackSchedule;
	
	for (const auto& [UserID, PassKind, Round, RewardGroup, StartTime, EndTime, PassState] : vecScheduleUpdate)
	{
		CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind);
		if (spBattlePass != nullptr)
		{
			vecRollbackSchedule.emplace_back(std::make_tuple(false, spBattlePass->m_PassKind, spBattlePass->m_Round, spBattlePass->m_RewardGroupID, spBattlePass->m_StartTime, spBattlePass->m_EndTime, spBattlePass->m_PassState));
			
			spBattlePass->m_PassKind = PassKind;
			spBattlePass->m_Round = Round;
			spBattlePass->m_PassState = PassState;
			spBattlePass->m_StartTime = StartTime;
			spBattlePass->m_EndTime = EndTime;
			spBattlePass->m_RewardGroupID = RewardGroup;

			spBattlePass->m_MissionMap.clear();
			spBattlePass->m_RewardMap.clear();
		}
		else
		{			
			spBattlePass = std::make_shared<CBattlePass>();
			if (spBattlePass == nullptr)
				throw std::bad_alloc();

			spBattlePass->m_PassKind = PassKind;
			spBattlePass->m_Round = Round;
			spBattlePass->m_PassState = PassState;
			spBattlePass->m_StartTime = StartTime;
			spBattlePass->m_EndTime = EndTime;
			spBattlePass->m_RewardGroupID = RewardGroup;

			spBattlePass->m_MissionMap.clear();
			spBattlePass->m_RewardMap.clear();

			ASE_INSTANCE(pUser, CBattlePassContainer)->Insert(spBattlePass);
			
			vecRollbackSchedule.emplace_back(std::make_tuple(true, spBattlePass->m_PassKind, spBattlePass->m_Round, spBattlePass->m_RewardGroupID, spBattlePass->m_StartTime, spBattlePass->m_EndTime, spBattlePass->m_PassState));
		}

		//이 로직은 진짜 배틀패스가 처음 열렸을 때 타는 로직이니까 퀘스트 초기 세팅을한다.
		if(PassState == BattlePass::ePassState::PROCESSING)
			CBattlePassHelper::RegistQuest(pUser, spBattlePass);
		
		ASE_INSTANCE(pUser, CBattlePassContainer)->SetUserID(pUser->UID());
	}

	Inventory& inventory = pUser->GetInventory();
	for (const auto& item : vecSetItem)
	{
		inventory.SetItemNum(item.Kind, item.Count);
	}
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_SCHEDULE_UPSERT_TV);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->TVP_1.clear(); // 배틀패스 스케쥴
			// INT64, INT32, INT32, INT32, BattlePass::eRewardType
			for (const auto& [UserID, PassKind, Round, RewardGroup, StartTime, EndTime, PassState] : vecScheduleUpdate)
			{
				pQuery->TVP_1.m_nPassKind.push_back(PassKind);
				pQuery->TVP_1.m_nRound.push_back(Round);
				pQuery->TVP_1.m_nRewardGroup.push_back(RewardGroup);
				pQuery->TVP_1.m_nStartTime.push_back(TLDB::TIME_UTC2DB(StartTime));
				pQuery->TVP_1.m_nEndTime.push_back(TLDB::TIME_UTC2DB(EndTime));
				pQuery->TVP_1.m_nState.push_back(static_cast<INT16>(PassState));
			}

			pQuery->TVP_2.clear(); // 배틀패스 아이템
			for (const auto& item : vecSetItem)
			{
				pQuery->TVP_2.m_nItemKind.push_back(item.Kind);
				pQuery->TVP_2.m_nItemCount.push_back(item.Count);
				pQuery->TVP_2.m_nItemCount_Svr.push_back(0);
			}
		
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()/*, vecScheduleRollback*/]()->RECV_RESULT
			{
				if (const auto pResultUser = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_SCHEDULE_UPSERT_TV. Fail Result:{}", set_1[0].m_nResult);

						for (const auto& [isNew, PassKind, Round, RewardGroup, StartTime, EndTime, PassState] : vecRollbackSchedule)
						{
							if (CBattlePass::SharedPtr rollBackBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(PassKind); rollBackBattlePass != nullptr)
							{
								if (IS_TRUE(isNew))
								{
									ASE_INSTANCE(pUser, CBattlePassContainer)->Delete(PassKind);
								}
								else
								{
									rollBackBattlePass->m_PassKind = PassKind;
									rollBackBattlePass->m_Round = Round;
									rollBackBattlePass->m_PassState = PassState;
									rollBackBattlePass->m_StartTime = StartTime;
									rollBackBattlePass->m_EndTime = EndTime;
									rollBackBattlePass->m_RewardGroupID = RewardGroup;
								}
							}
						}

						Inventory& rollbackInventory = pUser->GetInventory();
						for (const auto& item : vecRollBackItem)
						{
							rollbackInventory.SetItemNum(item.Kind, item.Count);
						}
						
						return  RECV_OK;
					}

					ValidProductContainer::UpdateProductInfos needUpdateProduct;
					for (const auto& [UserID, PassKind, Round, RewardGroup, StartTime, EndTime, PassState] : vecScheduleUpdate)
					{
						if (PassState == BattlePass::ePassState::DAYOFF)
							continue;

						// 유효한 상품 테이블에 등록
						const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(PassKind);
						if (passInfo == nullptr)
							continue;

						if (passInfo->NeedGem > 0)
							continue;

						if (passInfo->PackageKind == 0 || passInfo->PackageGroupCode == 0)
							continue;

						ValidProduct::SharedPtr spValiProduct = ASE_INSTANCE(pResultUser, ValidProductContainer)->Seek(eVALID_PRODUCT_TYPE::PACKAGE, passInfo->PackageKind, passInfo->PackageGroupCode);
						if (spValiProduct != nullptr)
						{
							// 아직 진행중인 데이터라면 변경 불가
							if (spValiProduct->isProcessing(curTime))
							{
								LOGGER_ERROR(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_SCHEDULE_UPSERT_TV ValidProduct Faild. PassKind : {}, Round : {}, PassStartTime : {}, PassEndTime : {}, Valid ProductKind : {}, Valid StartTime : {}, Valid EndTime : {}"
									, PassKind, Round, StartTime, EndTime, spValiProduct->productKind, spValiProduct->startTime, spValiProduct->endTime);

								continue;
							}
						}
						else
						{
							spValiProduct = std::make_shared<ValidProduct>();
							if (spValiProduct == nullptr)
								throw std::bad_alloc();

							spValiProduct->productType = eVALID_PRODUCT_TYPE::PACKAGE;
							spValiProduct->productKind = passInfo->PackageKind;
							spValiProduct->groupCode = passInfo->PackageGroupCode;

							ASE_INSTANCE(pUser, ValidProductContainer)->Insert(spValiProduct);
						}

						spValiProduct->startTime = StartTime;
						spValiProduct->endTime = EndTime;

						needUpdateProduct.emplace_back(std::make_tuple(spValiProduct->productType, spValiProduct->productKind, spValiProduct->groupCode, spValiProduct->startTime, spValiProduct->endTime, spValiProduct->precedenceGroupCode));
					}

					if (needUpdateProduct.size() > 0)
						ValidProductHelper::P_USER_VALID_PRODUCT_INFO_UPSERT_TV(userID, needUpdateProduct);


					// 클라로 노티해주자
					if (!vecSetItem.empty())
					{
						NEW_FLATBUFFER(GS_ITEM_SYNC_NFY, pPACKET);
						pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
						{
							std::vector<flatbuffers::Offset<PROTOCOL::FLATBUFFERS::ITEM>> itemList;
							for (const auto& item : vecSetItem)
								itemList.emplace_back(PROTOCOL::FLATBUFFERS::CreateITEM(fbb, item.Kind, item.Count, 0,0,0,0));
							
							return PROTOCOL::FLATBUFFERS::CreateGS_ITEM_SYNC_NFY(fbb, fbb.CreateVector(itemList));
						});
						SEND_ACTIVE_USER(pResultUser, pPACKET);
					}
				}

				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});
	
	return false;
}

bool CBattlePassHelper::P_BATTLEPASS_ADD_PASS_POINT(CUser* const pUser, const INT32 passKind, const INT32 passRound, const BattlePass::ePassType passType, const INT64 totalPassPoint, const INT32 deleteItemKind, const INT32 deleteItemCount)
{
	if (pUser == nullptr)
		return false;

	//@ServerID @UserID @PassKind	@Round	@PassType @PassPoint @DeleteItems
	const INT64 userID = pUser->UID();
	
	CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
		return false;

	if (passRound != spBattlePass->m_Round)
		return false;

	// 인멤 선처리
	if (IS_NULL(spBattlePass->m_spPoint))
	{
		spBattlePass->m_spPoint = std::make_shared<CBattlePassPoint>();
		if (spBattlePass->m_spPoint == nullptr)
			throw std::bad_alloc();
	}

	INT64 curPoint = spBattlePass->m_spPoint->m_curPoint;

	spBattlePass->m_spPoint->m_PassKind = spBattlePass->m_PassKind;
	spBattlePass->m_spPoint->m_Round = spBattlePass->m_Round;
	spBattlePass->m_spPoint->m_PassType = passType;
	spBattlePass->m_spPoint->m_curPoint = totalPassPoint;

	Inventory& inventory = pUser->GetInventory();
	INT64 setNum = inventory.GetItemNum(deleteItemKind) - deleteItemCount;
	inventory.SetItemNum(deleteItemKind, setNum);
	
	
	::OnRemoteDBA([=](const INT16 ssnid)->RECV_RESULT
		{
			BEGIN_GDB_QUERY_AUTO(P_BATTLEPASS_ADD_PASS_POINT);
			pQuery->m_nServerID = GLOBAL::GS_INFO.SVID;
			pQuery->m_nUserID = userID;
			pQuery->m_nPasskind = passKind;
			pQuery->m_nRound = passRound;
			pQuery->m_nPassType = static_cast<INT32>(passType);
			pQuery->m_nPassPoint = totalPassPoint;
			pQuery->TVP_1.clear(); // 미션 데이터
			pQuery->TVP_1.m_nItemKind.push_back(deleteItemKind);
			pQuery->TVP_1.m_nItemCount.push_back(deleteItemCount);
			pQuery->TVP_1.m_nItemCount_Svr.push_back(0);
		
			RUN_GDB_QUERY_AND_ERROR_RETURN_AUTO(RECV_FAIL);
			::OnRemoteGAME(ssnid, [=, set_1 = pQuery->GetSET_1()/*, vecMissionRollback*/]()->RECV_RESULT
			{
				if (auto pUser2 = CUserManager::Instance()->FindByUID(userID))
				{
					if (set_1.size() <= 0 || IS_FAILED(set_1[0].m_nResult))
					{
						if (set_1.size() > 0)
							LOGGER_DEBUG(CONST_BATTLEPASS_LOG, L"P_BATTLEPASS_MISSION_UPSERT_TV. Fail Result:{}", set_1[0].m_nResult);

						//for (const auto& [PassKind, Round, MissionKind, CurValue, State] : vecMissionRollback)
						//{

						//}

						return  RECV_OK;
					}

					auto curTier = curPoint / BattlePass::TIER_TO_POINT;
					auto targetTier = totalPassPoint / BattlePass::TIER_TO_POINT;
					auto curType = spBattlePass->m_spPoint->m_PassType;
					if (targetTier > curTier)
					{
						GLOBAL::SendLog(pUser->UID(), 0, DB_LOG::REASON_BATTLEPASS_TIER_UP, 0, 0,
							{ passKind, passRound, curTier,targetTier ,totalPassPoint, static_cast<INT64>(curType) }, {});
					}

					GLOBAL::SendLogItemUse(pUser->UID(), DB_LOG::REASON_BATTLEPASS_PASS_PIECE_ITEM_USE, deleteItemKind, deleteItemCount, { passKind, passRound, curTier, targetTier, curPoint, totalPassPoint - curPoint, totalPassPoint }, {});
					
					NEW_FLATBUFFER(GS_BATTLEPASS_POINT_NFY, pPACKET);
					pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
					{
						return PROTOCOL::FLATBUFFERS::CreateGS_BATTLEPASS_POINT_NFY(fbb, passKind, passRound, totalPassPoint, static_cast<INT32>(passType),
							PROTOCOL::FLATBUFFERS::CreateITEM(fbb, deleteItemKind, deleteItemCount, 0, 0, 0, 0));
					});
					SEND_ACTIVE_USER(pUser, pPACKET);
					
				}

				return  RECV_OK;
			});

			END_GDB_QUERY();
			return RECV_OK;
		});
	

	return false;
}


void CBattlePassHelper::SetBuffItem(INT64 i64UserID, const INT32 passKind, const INT32 iItemKind)
{
	CUser* pUser = CUserManager::Instance()->FindByUID(i64UserID);
	if (IS_NULL(pUser))
		return;

	auto pBuffCtrl = pUser->GetBuffController();
	if (IS_NULL(pBuffCtrl))
		return;

	auto pItemInfo = BASE::GET_ITEM_DATA(iItemKind);
	if (IS_NULL(pItemInfo))
		return;
	
	auto  buff_content_type = pItemInfo->GetBuffContentsType();
	INT32 buffLevel         = pItemInfo->i32VALUE[1];
	INT32 itemGroup         = pItemInfo->GetActiveBuffControllerItemGroup();
	INT64 workID            = 0;

	auto& activeBuffCtrler = pUser->GetActiveBuffController(buff_content_type);
	if (pItemInfo->i32ITEM_TYPE != GAME::ITEM_TYPE_BUFF)
		return;


	INT64 curGameTime = GLOBAL::GetGameTimeDueDay(0);
	INT64 startGameTime = 0;
	INT64 endGameTime = 0;

	const auto curTime = GetDueDay_UTC(0);
	
	const CBattlePass::SharedPtr spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->Seek(passKind);
	if (spBattlePass == nullptr)
		return;
	
	if (spBattlePass->m_PassState != BattlePass::ePassState::PROCESSING)
		return;
	
	startGameTime = curGameTime - (curTime - spBattlePass->m_StartTime);
	endGameTime = curGameTime + (spBattlePass->m_EndTime - curTime);

	auto& expireElem = (0 < itemGroup) ? activeBuffCtrler.GetElemByGroup(itemGroup) : activeBuffCtrler.GetElem(pItemInfo->i32ITEM_KIND);
	if (0 < expireElem.kind)
	{
		auto pFindWork = pUser->Territory().FindWork([&](CWork* pWork)->bool { return pWork->GetCommandType() == GAME::eCOMMAND_TYPE::ITEM_PERIOD_USE && pWork->GetKind() == expireElem.kind; });
		workID = (IS_NOT_NULL(pFindWork)) ? pFindWork->GetIDX() : 0;

		auto pDisuseItemInfo = BASE::GET_ITEM_DATA(expireElem.kind);
		if (IS_NOT_NULL(pDisuseItemInfo))
		{
			for (int i = 0; i < pDisuseItemInfo->GetBuffElemCount(); ++i)
			{
				auto pElem = pDisuseItemInfo->GetBuffElem(i);
				if (IS_NULL(pElem))
				{
					continue;
				}

				pBuffCtrl->DeleteBuff(buff_content_type, pElem->UID, true);
			}

			if (pDisuseItemInfo->i32ITEM_KIND != pItemInfo->i32ITEM_KIND)
			{
				NEW_FLATBUFFER(DB_BUFF_UPDATE_REQ, pPACKET);
				pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
				{
					auto buff = PROTOCOL::FLATBUFFERS::CreateBUFF(fbb,
						pUser->UID(),
						buff_content_type,
						pDisuseItemInfo->i32ITEM_KIND,
						expireElem.level,
						expireElem.startTime,
						GLOBAL::GetGameTimeDueDay(0));

					return PROTOCOL::FLATBUFFERS::CreateDB_BUFF_UPDATE_REQ(fbb,
						pUser->UID(),
						GLOBAL::GS_INFO.SVID,
						pDisuseItemInfo->i32ITEM_KIND,
						buff);
				});
				SEND_DBA(pPACKET);
			}
		}
	}

	for (int i = 0; i < pItemInfo->GetBuffElemCount(); ++i)
	{
		auto pBuffElem = pItemInfo->GetBuffElem(i);
		if (IS_NULL(pBuffElem))
		{
			continue;
		}

		pBuffCtrl->CreateBuff(buff_content_type,
			pBuffElem->buffProperty(),
			BASE::MAKE_BUFF_QUANTITY(pBuffElem, buffLevel, true),
			pBuffElem->CALCTYPE);
	}

	NEW_FLATBUFFER(DB_WORK_ITEM_PERIOD_SET_REQ, pPACKET_ITEM);
	pPACKET_ITEM.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{
		return PROTOCOL::FLATBUFFERS::CreateDB_WORK_ITEM_PERIOD_SET_REQ(fbb,
			GLOBAL::GS_INFO.SVID,
			pUser->UID(),
			workID,
			GAME::eCOMMAND_TYPE::ITEM_PERIOD_USE,
			pItemInfo->i32ITEM_KIND,
			startGameTime,
			endGameTime,
			itemGroup,
			endGameTime - startGameTime,
			0);
	});
	SEND_DBA(pPACKET_ITEM);

	NEW_FLATBUFFER(DB_BUFF_SET_REQ, pPACKET_BUFF);
	pPACKET_BUFF.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{
		auto buff = PROTOCOL::FLATBUFFERS::CreateBUFF(fbb,
			pUser->UID(),
			buff_content_type,
			pItemInfo->i32ITEM_KIND,
			buffLevel,
			startGameTime,
			endGameTime);

		return PROTOCOL::FLATBUFFERS::CreateDB_BUFF_SET_REQ(fbb,
			pUser->UID(),
			GLOBAL::GS_INFO.SVID,
			buff);
	});
	SEND_DBA(pPACKET_BUFF);

	NEW_FLATBUFFER(GS_ITEM_USE_ACK, pPACKET);
	pPACKET.Build([&](flatbuffers::FlatBufferBuilder& fbb)->auto
	{
		return PROTOCOL::FLATBUFFERS::CreateGS_ITEM_USE_ACK(fbb,
			0,
			PROTOCOL::FLATBUFFERS::CreateITEM(fbb, iItemKind, 0, 0, 0, endGameTime - startGameTime, buffLevel),
			NULL, startGameTime, endGameTime, expireElem.kind,
			PROTOCOL::FLATBUFFERS::CreateASSET(fbb, pUser->GetAssets().GetOil(), pUser->GetAssets().GetIron(), pUser->GetAssets().GetSilver(), pUser->GetAssets().GetGold()),
			PROTOCOL::FLATBUFFERS::CreateASSET(fbb, 0LL, 0LL, 0LL, 0LL),
			0, 0, 0, true);
	});
	SEND_ACTIVE_USER(pUser, pPACKET);

	pUser->UserUpdateStats();
	pUser->SendBattlePower();
}

bool CBattlePassHelper::IsApplyADSkipBuff_By_PrimePass(INT64 i64UserID)
{
	auto pUser = CUserManager::Instance()->FindByUID(i64UserID);
	if (IS_NULL(pUser) || pUser->IsPC() == false)
		return false;
	

	auto& spBattlePass = ASE_INSTANCE(pUser, CBattlePassContainer)->GetPassRepository();
	for (const auto& [passKind, passSchedule] : spBattlePass)
	{
		const auto& pUserPass = passSchedule->pBattlePass;
		if (pUserPass == nullptr)
			continue;
		
		const auto passInfo = BattlePass::CBattlePassManager::Instance()->GetPassInfo(passKind);
		if (passInfo == nullptr)
			continue;
		
		const auto passScheduleInfo = BattlePass::CBattlePassManager::Instance()->GetSchedule(passKind);
		if (passScheduleInfo == nullptr)
			continue;

		if (pUserPass->m_PassState == BattlePass::ePassState::DAYOFF)
			continue;

		const auto userPassTpye = pUserPass->m_spPoint == nullptr ? BattlePass::ePassType::FREE : pUserPass->m_spPoint->m_PassType;
		if (userPassTpye == BattlePass::ePassType::FREE)
			continue;
		
		if (BattlePass::CBattlePassManager::Instance()->CheckPassBuffStat(passKind, BuffContents::eBUFF_STAT_KIND::ADVERTISEMENT_SKIP))
			return true;
	}

	return false;
}




