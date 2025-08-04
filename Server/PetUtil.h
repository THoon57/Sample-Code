#pragma once

 
namespace Util::DragonPet
{
	template <typename _Ty>
	void SetData(const UserSharedPtr& in_game_user, const _Ty& in_row);

	auto GetDragonPetGainItemInfo(const Int32& _itemKind) -> NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr;

	bool GetRandomDragonPet(__out std::tuple<UInt32, UInt16, Int32>& _inPet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo);
	bool GetSelectDragonPet(__out std::tuple<UInt32, UInt16, Int32>& _inPet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo);
	auto SetPetRandomSkill(const DragonPetSharedPtr& _pet) -> Int32;
	auto SetPetSelectSkill(const DragonPetSharedPtr& _pet, NDT::COMMON_DRAGON_PET_INFO::DRAGONPET_GAINITEM_INFO::SharedPtr& _gainInfo) -> Int32;

	auto GetDragonPetMaxFeedingCount(const DragonPetSharedPtr in_Pet) -> Int32;
	auto GetDragonPetFeedingBaseCooltime(const DragonPetSharedPtr in_pet) -> Int64;
	auto GetDragonPetFeedingCooltime(const UserBaseSharedPtr& in_game_user, const DragonPetSharedPtr& in_pet) -> Int64;
	void InitializeDragonPetSatiety(const UserBaseSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack, const DragonPetSharedPtr in_pet);
	auto CreateDragonPetFeedingCooltime(const UserBaseSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack, const DragonPetSharedPtr in_pet) -> bool;
	auto ValidPromote(const DragonPetSharedPtr& in_targetPet, const int& _percent) -> bool;
	void GetDragonPetPassiveSkillBuffs(const DragonPetSharedPtr& in_pet, __out std::vector<core::ObjID>& out_result);

	auto SimulateDragonPetAdventure(std::vector<core::ObjID> in_rewards) -> const core::ObjID;
	bool CheckDragonPetHatchingItem(const Int32& in_item_kind, const Int32& in_select_pet_kind);
	auto GetSkillAddValue(const Int32& _skillKind, const Int32& _skillLevel, const Int32& _skillValue) -> Int32;
	auto GetSkillUniqeKind(const Int32& _skillKind, std::vector<core::ObjID>& _skillGroups) -> Int32;
	bool ValidSkillLevelUp(const Int32& _skillKind, const Int32& _skillLevel);
	bool IsValidSkillGroup(const Int32& _skillGroup, const Int32& _skillKind);
	auto GetSkillGrade(const Int32& _skillGroup, const Int32& _skillKind) -> Int32;
	auto GetSkillGroup(const Int32& _skillKind) -> Int32;

	auto GetRandomSkillKind(const Int32& _skillGroup) -> Int32;
	auto GetSelectSkillKind(const Int32& _skillGroup, const Int32& in_skillGrade) -> Int32;
	auto GetRandomSkillBuffKind(const Int32& in_skillGroup, const Int32& in_skillKind) -> Int32;

	void UpdatePetTraining(const UserSharedPtr& in_game_user, const QPackSharedPtr& in_query_pack);

	auto GetEquipPetID(const UserBaseSharedPtr& in_game_user, const Int64& in_heroID) -> Int64;
}
