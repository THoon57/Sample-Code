#include "GameServer.h"
#include "Util-DragonPet.h"

namespace Util::DragonPet
{
	template void SetData(const UserSharedPtr& in_game_user, const QP_DRAGONPET_FETCH_SERVER::tagSET_1& in_query);
	template void SetData(const UserSharedPtr& in_game_user, const QP_DRAGONPET_FETCH_USER::tagSET_1& in_query);
	template <typename _Ty>
	void SetData(const UserSharedPtr& in_game_user, const _Ty& in_row)
	{
		auto dragonPet = std::make_shared<CDragonPetObject>();
		dragonPet->SetUserID(in_row.m_nUserID);
		dragonPet->SetGameObjID(core::ObjID(core::ContentID(EnumCategory::DragonPet, in_row.m_nKIND), in_row.m_nPetID));
		dragonPet->SetLV(in_row.m_nLV);
		dragonPet->SetGradeLV(in_row.m_nGradeLV);
		dragonPet->SetEXP(in_row.m_nEXP);
		dragonPet->SetSatiety(in_row.m_nSatiety);
		dragonPet->SetSkillProperties(in_row.m_szSkillProperties);
		dragonPet->SetName(in_row.m_szName);
		dragonPet->SetRarity(in_row.m_nRarity);
		dragonPet->SetTraining(in_row.m_nTraining);
		dragonPet->SetLocked(in_row.m_nLocked);
		dragonPet->SetHeroID(in_row.m_nHeroID);

		ASE_INSTANCE(in_game_user, CUserDragonPetManager)->Insert(dragonPet);
	}

	
	auto GetDragonPetGainItemInfo(const Int32& _itemKind) -> NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr
	{	
		const auto& item_info = NDT::COMMON_ITEM_INFO::TABLE::Seek_KIND(_itemKind);
		if (nullptr == item_info)
			return nullptr;

		if (item_info->Properties.size() <= 0)
			return nullptr;

		//여기도 조각 획득 말고 다른거 나오면 대응 해줘야함.
		auto gainKind = item_info->Properties[0].GetKIND();
		if (gainKind <= 0)
			return nullptr;

		auto gainInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SeekByKIND(gainKind);
		if (gainInfo == nullptr)
			return nullptr;

		return gainInfo;
	}

	bool GetRandomDragonPet(std::tuple<UInt32, UInt16, Int32>& _inPet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo)
	{
		auto randomValue = core::PERMYRIAD<INT32>();

		if (_gainInfo == nullptr)
		{
			_inPet = std::make_tuple<UInt32, UInt16, Int32>(0, 0, 0);
			return false;
		}

		for (const auto& gain : _gainInfo->Properties)
		{
			if (randomValue > gain.GetRANDCount())
				randomValue -= gain.GetRANDCount();
			else
			{
				_inPet = std::make_tuple<UInt32, UInt16, Int32>(gain.GetKIND(), core::GetVariety(gain.identifier), gain.GetPercent());
				return true;
			}

		}


		_inPet = std::make_tuple<UInt32, UInt16, Int32>(0, 0, 0);
		return false;
	}

	bool GetSelectDragonPet(std::tuple<UInt32, UInt16, Int32>& _inPet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo)
	{
		if (_gainInfo == nullptr)
		{
			_inPet = std::make_tuple<UInt32, UInt16, Int32>(0, 0, 0);
			return false;
		}

		for (const auto& gain : _gainInfo->Properties)
		{
			_inPet = std::make_tuple<UInt32, UInt16, Int32>(gain.GetKIND(), core::GetVariety(gain.identifier), gain.GetPercent());
			return true;
		}

		return false;
	}

	auto SetPetRandomSkill(const DragonPetSharedPtr& _pet) -> Int32
	{
		if (_pet == nullptr)
			return 0;

		auto petInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_KIND_INFO::SeekByKIND(_pet->GetKIND());
		if (petInfo == nullptr)
			return 0;

		auto skillGrade = 0;
		auto petGrade = _pet->GetGradeLV();
		Int32 randomValue = 0;

		for (const auto& skillInfo : petInfo->SkillGroup)
		{
			auto skillGroupKind = skillInfo.GetKIND();

			auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(skillGroupKind);
			if (skillGroup.size() <= 0)
				return 0;

			auto rateToTal = 0;
			for (const auto& group : skillGroup)
				rateToTal += group->Rate;

			if (rateToTal <= 0)
				return 0;

			randomValue = core::RAND(rateToTal - 1);

			auto checkValue = 0;
			for (auto& group : skillGroup)
			{
				checkValue += group->Rate;
				if (randomValue < checkValue)
				{
					auto buffCheckValue = 0;
					auto buffCheckRateTotal = 0;
					auto buffRandomValue = 0;

					for (auto& buffRate : group->ProPerties)
						buffCheckRateTotal += buffRate.first;

					buffRandomValue = core::RAND(buffCheckRateTotal - 1);

					for (auto& pro : group->ProPerties)
					{
						buffCheckValue += pro.first;
						if (buffRandomValue < buffCheckValue)
						{
							auto skillLevel = 1;
							auto skillKind = pro.GetKIND();
							const auto& buffData = NDT::COMMON_BUFF_INFO::TABLE::Seek_KIND(skillKind);
							if (buffData == nullptr)
								continue;

							auto skillValue = 0;

							for (const auto& [key, data] : buffData->Properties)
							{
								skillValue = data.GetValue();
								break;
							}

							for (; skillLevel < petGrade; ++skillLevel)
							{
								const auto& buffData = NDT::COMMON_BUFF_INFO::TABLE::Seek_KIND(skillKind);
								if (buffData == nullptr)
									continue;

								skillValue += buffData->IndexMultiplier;
							}


							_pet->AddSkill(CDragonPetObject::SKILL(pro.GetKIND(), skillLevel, skillValue, group->SkillGrade));
							if (skillGrade < group->SkillGrade)
								skillGrade = group->SkillGrade;

							break;
						}
					}
					break;
				}
			}
		}

		return skillGrade;
	}

	auto SetPetSelectSkill(const DragonPetSharedPtr& _pet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo) -> Int32
	{
		if (_gainInfo == nullptr)
			return 0;

		auto result = 0;

		auto petGrade = _pet->GetGradeLV();

		auto skillGroup = 0;
		auto skillGrade = 0;
		auto skillKind  = 0;
		auto skillLevel = 1;
		auto skillValue = 0;


		CDragonPetObject::SKILL addSkill;
		for (const auto& grade : _gainInfo->SkillGrade)
		{
			skillGroup = grade.GetKIND();
			skillGrade = core::GetVariety(grade.identifier);
			skillKind = GetSelectSkillKind(skillGroup, skillGrade);

			const auto& buffData = NDT::COMMON_BUFF_INFO::TABLE::Seek_KIND(skillKind);
			if (buffData == nullptr)
				continue;

			skillValue = 0;
			for (const auto& [key, data] : buffData->Properties)
			{
				skillValue = data.GetValue();
				break;
			}

			for (; skillLevel < petGrade; ++skillLevel)
			{
				const auto& buffData = NDT::COMMON_BUFF_INFO::TABLE::Seek_KIND(skillKind);
				if (buffData == nullptr)
					continue;

				skillValue += buffData->IndexMultiplier;
			}

			if (skillGrade == 0)
				skillGrade = GetSkillGrade(skillGroup, skillKind);

			_pet->AddSkill(CDragonPetObject::SKILL(skillKind, skillLevel, skillValue, skillGrade));

			if (result < skillGrade)
				result = skillGrade;

			skillLevel = 1;
		}

		return result;
	}

	auto GetDragonPetMaxFeedingCount(const DragonPetSharedPtr in_Pet) -> Int32
	{
		if (in_Pet == nullptr)
			return 0;

		auto growthInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GROWTH_TYPE_INFO::Seek_KINDGRADE(in_Pet->GetKIND(), in_Pet->GetGradeLV());
		if (growthInfo == nullptr)
			return 0;

		return growthInfo->FEEDING_COUNT;
	}

	auto GetDragonPetFeedingBaseCooltime(const DragonPetSharedPtr in_pet) -> Int64
	{
		if (nullptr == in_pet)
			return NDT::COMMON_DRAGON_PET_INFO::Const.DRAGONPET_DEFAULT_FEEDING_COOLTIME;

		const auto growth_info = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GROWTH_TYPE_INFO::Seek_KINDGRADE(in_pet->GetKIND(), in_pet->GetGradeLV());
		if (nullptr == growth_info)
			return NDT::COMMON_DRAGON_PET_INFO::Const.DRAGONPET_DEFAULT_FEEDING_COOLTIME;

		return growth_info->FEEDING_COOLTIME;
	}

	auto GetDragonPetFeedingCooltime(const UserBaseSharedPtr& in_game_user, const DragonPetSharedPtr& in_pet) -> Int64
	{
		// 기본 시간은 드래곤마다 가져와야 한다
		auto feeding_cooltime_sec = GetDragonPetFeedingBaseCooltime(in_pet);

		if (nullptr == in_game_user)
			return feeding_cooltime_sec;

		// 유저 버프 적용
		const auto user_buff_value = BuffContents::Common::GetBuffValue(in_game_user, BuffContents::TIMECOOL_Pet);
		if (false == user_buff_value.IsEmpty()) {
			const auto buffed_msec = BuffContents::CalcTimeResult(user_buff_value, feeding_cooltime_sec);
			if (0 < buffed_msec) {
				feeding_cooltime_sec = buffed_msec / MSEC_ONE_SECOND;
			}
		}

		return feeding_cooltime_sec;
	}

	void InitializeDragonPetSatiety(const UserBaseSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack, const DragonPetSharedPtr in_pet)
	{
		in_pet->SetSatiety(GetDragonPetMaxFeedingCount(in_pet));
		CreateDragonPetFeedingCooltime(in_game_user, in_query_pack, in_pet);
	}

	auto CreateDragonPetFeedingCooltime(const UserBaseSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack, const DragonPetSharedPtr in_pet) -> bool
	{
		if (in_game_user == nullptr || in_pet == nullptr)
			return false;

		if (in_game_user->IsAI())
			return true;

		if (0 >= in_pet->GetSatiety())
			return true;

		const int  coolTime   = GetDragonPetFeedingCooltime(in_game_user, in_pet);
		const auto feddingTto = CTTo_DragonPetFeeding::Seek(in_game_user->GetGameObjID(), in_pet->GetGameObjID());

		// 이미 포만감 TTO가 돌고 있는 경우, true로 나간다
		if (feddingTto != nullptr)
			return true;

		// 비정상적인 포만감 쿨타임이므로 종료
		if (coolTime <= 0)
			return false;

		const CContentPacker packer(in_query_pack);
		// 포만감 쿨타임 생성
		const auto new_tto = CTTo_DragonPetFeeding::MakeTimedTask(packer, in_game_user->GetGameObjID()
			, in_pet->GetGameObjID()
			, coolTime);

		if (new_tto == nullptr)
			return false;

		packer.CheckSend();

		return true;
	}

	auto ValidPromote(const DragonPetSharedPtr& in_targetPet, const int& _percent) -> bool
	{
		if (in_targetPet == nullptr)
			return false;

		//타겟 펫 검사
		const auto targetPetcurGradeInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GRADE_INFO::Seek_KINDGRADE(in_targetPet->GetKIND(), in_targetPet->GetGradeLV());
		if (targetPetcurGradeInfo == nullptr)
			return false;

		if (targetPetcurGradeInfo->MaxLevel > in_targetPet->m_lv)
			return false;

		const auto targetPetnextGradeInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GRADE_INFO::Seek_KINDGRADE(in_targetPet->GetKIND(), in_targetPet->GetGradeLV() + 1);
		if (targetPetnextGradeInfo == nullptr)
			return false;

		const auto targetPetInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_KIND_INFO::Seek_KIND(in_targetPet->GetKIND());
		if (targetPetInfo == nullptr)
			return false;

		const auto targetPetRateInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_PROMOTE_TRY_RATE::Seek_Grade(in_targetPet->GetGradeLV());
		if (targetPetRateInfo == nullptr)
			return false;

		for (const auto percent : targetPetRateInfo->Percent) {
			if (percent == _percent)
				return true;
		}

		return true;
	}

	void GetDragonPetPassiveSkillBuffs(const DragonPetSharedPtr& in_pet, __out std::vector<core::ObjID>& out_result)
	{
		if (false == Util::ContentState::IsActive(EnumContentState::DRAGONPET))
			return;

		if (nullptr == in_pet) 
			return;

		for (const auto& passive_skill : in_pet->GetSkill()) 
		{
			auto level = std::max(0, passive_skill.level - 1);
			core::ObjID buff_obj_id = core::MakeObjID(core::MakeContentID(EnumCategory::IndexBuff, passive_skill.kind), static_cast<UInt64>(level));
			out_result.emplace_back(buff_obj_id);
		}
	}

	auto SimulateDragonPetAdventure(std::vector<core::ObjID> in_rewards) -> const core::ObjID
	{
		// 만분률로 뽑는다
		const auto factor = core::PERMYRIAD<Int32>();

		auto Amount = 0;

		core::ObjID selected_reward = core::ObjID::npos;

		for (Int32 index = 0; index < in_rewards.size(); ++index)
		{
			Amount += in_rewards[index].second;
			if (factor < Amount)
			{
				selected_reward = in_rewards[index];
				break;
			}
		}

		return selected_reward;
	}

	bool CheckDragonPetHatchingItem(const Int32& in_item_kind, const Int32& in_select_pet_kind)
	{
		const auto& item_info = NDT::COMMON_ITEM_INFO::TABLE::Seek_KIND(in_item_kind);
		if (nullptr == item_info)
			return false;

		if (in_item_kind == NDT::COMMON_DRAGON_PET_INFO::Const.CONST_DRAGONPET_NORMALEGG_KIND)
			return true;

		if (1 != item_info->Properties.size() || EnumCategory::DragonPet != static_cast<EnumCategory>(item_info->Properties[0].GetCategory()))
			return false;

		const auto gain_item = item_info->Properties[0];
		auto petGainInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::Seek_KIND(gain_item.GetKIND());
		if (petGainInfo == nullptr)
			return false;

		if (in_select_pet_kind == 0)
			return true;


		return false;
	}

	auto GetSkillAddValue(const Int32& _skillKind, const Int32& _skillLevel, const Int32& _skillValue) -> Int32
	{
		const auto& skillStatInfos = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_STAT_INFO::ListByKINDSkillLevel(_skillKind, _skillLevel);
		if (skillStatInfos.empty() == true)
			return 0;

		for (auto& skillStat : skillStatInfos)
		{
			if (skillStat == nullptr)
				continue;

			if (skillStat->SkillStatRange.minval <= _skillValue && skillStat->SkillStatRange.maxval >= _skillValue)
				return skillStat->SkillLevelEnchantCount;
		}

		return 0;
	}

	auto GetSkillUniqeKind(const Int32& _skillKind, std::vector<core::ObjID>& _skillGroups) -> Int32
	{
		if (_skillGroups.empty() == true)
			return 0;

		UInt32 groupKind = 0;
		for (auto& group : _skillGroups)
		{
			groupKind = group.GetKIND();
			auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(groupKind);
			if (skillGroup.size() <= 0)
				return 0;

			for (auto& skill : skillGroup)
			{
				for (auto& pro : skill->ProPerties)
				{
					if (pro.GetKIND() == _skillKind)
						return skill->KIND;
				}
			}
		}

		return 0;
	}

	bool ValidSkillLevelUp(const Int32& _skillKind, const Int32& _skillLevel)
	{
		const auto& buffData = NDT::COMMON_BUFF_INFO::TABLE::Seek_KIND(_skillKind);
		if (buffData == nullptr)
			return false;

		if (_skillLevel > buffData->IndexDepth)
			return false;

		return true;
	}

	bool IsValidSkillGroup(const Int32& _skillGroup, const Int32& _skillKind)
	{
		auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(_skillGroup);
		if (skillGroup.size() <= 0)
			return 0;

		for (const auto& group : skillGroup)
		{
			for (auto& buffRate : group->ProPerties)
			{
				 if(_skillKind == buffRate.GetKIND())
					return true;
			}
		}


		return false;
	}

	auto GetSkillGrade(const Int32& _skillGroup, const Int32& _skillKind) -> Int32
	{
		auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(_skillGroup);
		if (skillGroup.size() <= 0)
			return 0;

		for (const auto& group : skillGroup)
		{
			for (auto& buffRate : group->ProPerties)
			{
				if (_skillKind == buffRate.GetKIND())
					return group->SkillGrade;
			}
		}

		return 0;
	}

	auto GetSkillGroup(const Int32& _skillKind) -> Int32
	{
		auto skillGroupInfos = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::GetRepository();

		for (auto [key, data] : skillGroupInfos)
		{
			for (auto pro : data->ProPerties)
			{
				if (_skillKind == pro.GetKIND())
					return data->SkillGroup;
			}
		}

		return 0;
	}

	auto GetRandomSkillKind(const Int32& _skillGroup) -> Int32
	{
		auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(_skillGroup);
		if (skillGroup.size() <= 0)
			return 0;

		auto rateToTal = 0;
		for (const auto& group : skillGroup)
			rateToTal += group->Rate;

		if (rateToTal <= 0)
			return 0;

		auto randomValue = core::RAND(rateToTal - 1);

		auto checkValue = 0;
		for (auto& group : skillGroup)
		{
			checkValue += group->Rate;
			if (randomValue < checkValue)
			{
				auto buffCheckValue = 0;
				auto buffCheckRateTotal = 0;
				auto buffRandomValue = 0;

				for (auto& buffRate : group->ProPerties)
					buffCheckRateTotal += buffRate.first;

				buffRandomValue = core::RAND(buffCheckRateTotal - 1);

				for (auto& pro : group->ProPerties)
				{
					buffCheckValue += pro.first;
					if (buffRandomValue < buffCheckValue)
					{
						return pro.GetKIND();
					}
				}
				break;
			}
		}

		return 0;
	}
	auto GetSelectSkillKind(const Int32& _skillGroup, const Int32& in_skillGrade) -> Int32
	{
		auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(_skillGroup);
		if (skillGroup.size() <= 0)
			return 0;

		if (in_skillGrade == 0)
			return GetRandomSkillKind(_skillGroup);

		for (auto& group : skillGroup)
		{
			if (in_skillGrade != group->SkillGrade)
				continue;

			auto buffCheckValue = 0;
			auto buffCheckRateTotal = 0;
			auto buffRandomValue = 0;

			for (auto& buffRate : group->ProPerties)
				buffCheckRateTotal += buffRate.first;

			buffRandomValue = core::RAND(buffCheckRateTotal - 1);

			for (auto& pro : group->ProPerties)
			{
				buffCheckValue += pro.first;
				if (buffRandomValue < buffCheckValue)
				{
					return pro.GetKIND();
				}
			}
			break;

		}
		return 0;
	}
	auto GetRandomSkillBuffKind(const Int32& in_skillGroup, const Int32& in_skillKind) -> Int32
	{
		auto skillGroup = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::ListBySkillGroup(in_skillGroup);
		if (skillGroup.size() <= 0)
			return 0;

		auto skillInfo = NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_SKILL_INFO::Seek_SkillGroupKIND(in_skillGroup, in_skillKind);
		if (skillInfo == nullptr)
			return 0;

		auto buffCheckValue = 0;
		auto buffCheckRateTotal = 0;
		auto buffRandomValue = 0;

		for (auto& buffRate : skillInfo->ProPerties)
			buffCheckRateTotal += buffRate.first;

		buffRandomValue = core::RAND(buffCheckRateTotal - 1);

		for (auto& pro : skillInfo->ProPerties)
		{
			buffCheckValue += pro.first;
			if (buffRandomValue < buffCheckValue)
			{
				return pro.GetKIND();
			}
		}

		return 0;
	}
	void UpdatePetTraining(const UserSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack)
	{
		if (nullptr == in_game_user)
			return;

		if (in_game_user->IsAI())
			return;

		const auto announce = ASE_INSTANCE(in_game_user, CUserAnnounceManager)->Seek(EnumAnnounceKIND::PET_TRAINING_UPDATE, in_game_user->GetUserID());
		if (nullptr != announce && false == Util::Time::IsDayChangeFromNow(announce->GetUpdateTime()))
			return;

		const CContentPacker packer(in_query_pack);

		ASE_INSTANCE(in_game_user, CUserDragonPetManager)->ResetDragonPetTrainingCount(packer);

		packer << ASE_INSTANCE(in_game_user, CUserAnnounceManager)->Insert(EnumAnnounceKIND::PET_TRAINING_UPDATE, in_game_user->GetUserID());

		packer.CheckSend();
	}
	auto GetEquipPetID(const UserBaseSharedPtr& in_game_user, const Int64& in_heroID) -> Int64
	{
		Int64 result = 0;

		if (in_game_user == nullptr)
			return result;

		if (in_game_user->IsAI())
			return result;

		auto petRepo = ASE_INSTANCE(in_game_user, CUserDragonPetManager)->GetRepo();
		for (const auto& pet : petRepo)
		{
			if (pet == nullptr)
				continue;

			if (in_heroID == pet->GetHeroID())
			{
				result = pet->GetGameObjID().GetDatabaseID();
				break;
			}
		}

		return result;
	}
}

