using System;
using System.Collections;
using System.Collections.Generic;
using CORE.UI.Control;
using Framework.EventSystem;
using GameEvent;
using NDT;
using NETWORK.GAME;
using NGraphics;
using UnityEngine;


public sealed class GUI_DragonPetInfoScreen : ScreenDisplayObject
{
    [Header("Title")] 
    [SerializeField] public UILabel    m_label_dragon_title = null;
    [SerializeField] public Animator   m_animator_title     = null;
    [SerializeField] public UIButtonEx m_button_back        = null;
    [SerializeField] public UIButtonEx m_Button_Info        = null;

    [Header("Root Animator")]
    [SerializeField] public Animator m_animator_root_left = null;
    [SerializeField] public Animator m_animator_basic      = null;
    [SerializeField] public Animator m_animator_feeding    = null;
    [SerializeField] public Animator m_animator_skill_info = null;
    [SerializeField] public Animator m_animator_hatching   = null;
    [SerializeField] public Animator m_animator_promote    = null;

    [Header("Pet List")]
    [SerializeField] public UISheet        m_sheet_pet_list            = null;
    [SerializeField] public UILabel        m_label_sort_option         = null;
    [SerializeField] public UISprite       m_sprite_sort_option_arrow  = null;
    [SerializeField] public UIButtonEx     m_button_sort_option_close  = null;
    [SerializeField] public UIButtonEx     m_button_sort_option        = null;
    [SerializeField] public UIXRadioButton m_radio_button_sort_options = null;
    [SerializeField] public UILabel        m_label_sort_name_level     = null;
    [SerializeField] public UILabel        m_label_sort_name_atk       = null;
    [SerializeField] public UILabel        m_label_sort_name_def       = null;
    [SerializeField] public UILabel        m_label_sort_name_hp        = null;
    [SerializeField] public UILabel        m_label_pet_count           = null;


    [Header("Hatching Open")]
    [SerializeField] public UIButtonEx m_button_hatching_select  = null;
    [SerializeField] public UILabel     m_label_hatching_count   = null;
    [SerializeField] public UILabel     m_label_hatchable        = null;
    [SerializeField] public Sub_NewMark m_sub_hatching_new_mark  = null;
    [SerializeField] public Transform   m_hatch_able_effect      = null;

    [Header("Basic")]
    [SerializeField] public Sub_DragonBaseUI m_sub_base_ui_basic                 = null;
    [SerializeField] public UIButtonEx       m_button_promote_move               = null;
    [SerializeField] public UIButtonEx       m_button_name_change                = null;
    [SerializeField] public Animator         m_animator_exp_gauge                = null;
    [SerializeField] public UILabel          m_label_grade                       = null;
    [SerializeField] public TweenPosition    m_tween_promote_arrow               = null;
    [SerializeField] public UISprite         m_sprite_promote_arrow              = null;
    [SerializeField] public UILabel          m_label_basic_feeding_remain_time   = null;

    [SerializeField] public UIButtonEx m_button_go_dragon_adventure = null;

    // 훈련 버튼 있는버전
    [SerializeField] public Transform  m_root_button_group_train = null;
    [SerializeField] public UIButtonEx m_button_train            = null;
    [SerializeField] public UIButtonEx m_button_feeding          = null;
    [SerializeField] public UIButtonEx m_button_evolution        = null;
    [SerializeField] public UILabel    m_label_feeding_btn       = null;
    [SerializeField] public UILabel    m_label_evolution_btn     = null;

    // 훈련버튼 없는 버전
    [SerializeField] public Transform  m_root_button_group_non_trainin    = null;
    [SerializeField] public UIButtonEx m_button_feeding_non_training      = null;
    [SerializeField] public UIButtonEx m_button_evolution_non_training    = null;
    [SerializeField] public UILabel    m_label_feeding_btn_non_training   = null;
    [SerializeField] public UILabel    m_label_evolution_btn_non_training = null;

    [SerializeField] public UISheet    m_sheet_pet_stat                   = null;
    [SerializeField] public UISprite   m_Sprite_EquipHero                 = null;
    [SerializeField] public UISprite   m_Sprite_AddHero                   = null;
    [SerializeField] public UISprite   m_Sprite_EquipHeroGrade            = null;
    [SerializeField] public UILabel    m_Label_EquipHero                  = null;
    [SerializeField] public UIButton   m_Button_EquipHero                 = null;
    [SerializeField] public UILabel    m_Label_BattlePower                = null;
    [SerializeField] public UIButton   m_Button_BattlePower               = null;
    [SerializeField] public Sub_Tooltip_DetailBattlePoint m_sub_tool_tip_buff;


    [Header("Feeding")] 
    [SerializeField] public Sub_DragonBaseUI m_sub_base_ui_feeding              = null;
    [SerializeField] public UILabel          m_label_dragon_feeding_title       = null;
    [SerializeField] public UILabel          m_label_sympathize_info_item_empty = null;
    [SerializeField] public UISheet          m_sheet_feeding                    = null;
    [SerializeField] public UIButtonEx       m_button_sympathize_close          = null;
    [SerializeField] public UIButtonEx       m_button_go_combine                = null;


    [Header("SkillInfo")]
    [SerializeField] public Sub_Skill   m_pet_skill = null;
    [SerializeField] public UILabel     m_label_skill_info_name   = null;
    [SerializeField] public UILabel     m_label_skill_info_desc   = null;
    [SerializeField] public UIButtonEx  m_button_skill_info_close = null;

    [Header("Force Hatching")]
    [SerializeField] public UILabel    m_label_force_hatching = null;
    [SerializeField] public UIButtonEx m_button_force_hatching = null;

    [Header("Center")]
    [SerializeField] public UILabel        m_label_hatching_able_dragon = null;
    [SerializeField] public UILabel        m_label_hatching_gain_nest   = null;
    [SerializeField] public UILabel        m_label_egg_info             = null;
    [SerializeField] public UIWidget       m_widget_dragon_egg          = null;
    [SerializeField] public UIWidget       m_widget_model               = null;
    [SerializeField] public UIWidget       m_widget_alter_model         = null;
    [SerializeField] public Animator       m_animator_egg               = null;
    [SerializeField] public Animator       m_animator_dragon_model_view = null;
    [SerializeField] public Animator       m_animator_egg_model_view    = null;
    [SerializeField] public UIButtonEx     m_button_view_drag           = null;
    [SerializeField] public UIButtonEx     m_button_product             = null;
    [SerializeField] public AnimationCurve m_stop_easing_curve          = null;
    [SerializeField] public UILabel[]      m_label_stat_up              = null;
    [SerializeField] public Animator[]     m_animator_stat_up           = null;
    [SerializeField] public Animator       m_Ani_BattlePower            = null;
    [SerializeField] public Animator       m_Ani_EquipHero              = null;


    [Header("Bottom")]
    //[SerializeField] public UILabel          m_label_growth_step            = null;
    //[SerializeField] public UILabel[]        m_label_growth_step_need_level = new UILabel[7];
    //[SerializeField] public UISprite[]       m_sprite_growth_step_line      = new UISprite[6];
    //[SerializeField] public UISprite[]       m_sprite_growth_step           = new UISprite[7];
    //[SerializeField] public UISprite[]       m_sprite_growth_step_select    = new UISprite[7];
    //[SerializeField] public GameObject[]     m_root_growth_step_block       = new GameObject[5];
    //[SerializeField] public UISprite[]       m_sprite_growth_step_line_block  = new UISprite[4];
    //[SerializeField] public UIButtonEx[]     m_button_growth_step           = new UIButtonEx[6];
    //[SerializeField] public Animator         m_animator_growth_group        = null;
    //[SerializeField] public UIXToggleButton  m_toggle_button_favority       = null;
    //[SerializeField] public UIXToggleButton  m_toggle_button_notification   = null;
    [SerializeField] public UIXToggleButton  m_toggle_button_lock           = null;

    //[SerializeField] public DragonProductSlot[] m_draogn_product_slots            = new DragonProductSlot[3];
    //[SerializeField] public UIProgressBar       m_progress_basic_product_time     = null;
    //[SerializeField] public UISprite            m_sprite_progress_scale_icon      = null;
    //[SerializeField] public UIButtonEx          m_button_basic_product_time       = null;
    //[SerializeField] public UILabel             m_label_basic_product_remain_time = null;
    //[SerializeField] public UIButtonEx          m_btn_product_progress            = null;
    //[SerializeField] public Animator            m_animator_product_progress       = null;


    [Header("Promote")]
    [SerializeField] public Sub_DragonBaseUI m_sub_base_ui_promote                   = null;
    [SerializeField] public Animator         m_animator_promote_max_lv               = null;
    [SerializeField] public Animator[]       m_animator_skill_promote                = null;
    [SerializeField] public UIButtonEx       m_button_promote                        = null;
    [SerializeField] public UILabel          m_label_promote_button                  = null;
    [SerializeField] public UILabel          m_label_promote_current_max_level       = null;
    [SerializeField] public UILabel          m_label_promote_next_max_level          = null;
    [SerializeField] public UISprite         m_sprite_promote_next_arrow             = null;
    [SerializeField] public UILabel          m_label_notify_max_promote              = null;
    [SerializeField] public Sub_Probability  m_sub_probability                       = null;
    [SerializeField] public Sub_TryRate      m_sub_try_rate                          = null;
    [SerializeField] public GameObject       m_promote_sourceitem_object             = null;
    [SerializeField] public GameObject       m_promote_dragonstatinfo_object         = null;



    [Header("Hatching")]
    [SerializeField] public UILabel        m_label_hatching_piece_count_center  = null;
    [SerializeField] public UILabel        m_label_hatching_piece_count         = null;
    [SerializeField] public UIButtonEx     m_button_hatching_link               = null;
    [SerializeField] public UIButtonEx     m_button_hatching                    = null;
    [SerializeField] public UIButtonEx     m_button_hatching_multiple           = null;
    [SerializeField] public UIButtonEx     m_button_hatching_close              = null;
    [SerializeField] public UILabel        m_label_hatching_btn_title           = null;
    [SerializeField] public UILabel        m_label_multiple_hatching_btn_title  = null;
    [SerializeField] public UILabel        m_label_hatch_require_count          = null;
    [SerializeField] public UILabel        m_label_multiple_hatch_require_count = null;
    [SerializeField] public UISheet        m_sheet_hatching_pet                 = null;
    [SerializeField] public GameObject     m_camera_hatching                    = null;
    [SerializeField] public Transform      m_hatching_camera_parent             = null;

    [SerializeField] public UIXRadioButton m_Hatching_RatingButton              = null;
    [SerializeField] public UISprite       m_Sprite_HatchEgg                    = null;
    [SerializeField] public Sub_NewMark    m_Sub_NewMark_NormalEgg              = null;
    [SerializeField] public Sub_NewMark    m_Sub_NewMark_UniqueEgg              = null;

    //[펫UI 스킬배치] : 엑티브1 엑티브2 패시브1 랜덤패시브1
    [SerializeField] public UIGrid             m_grid;
    [SerializeField] public List<SkillSlot>    m_skills             = new List<SkillSlot>();

    [SerializeField] public GameObject         m_obj_probability    = null;
    [SerializeField] public UIButtonEx         m_button_probability = null;


    [SerializeField] public PetSlot[]          m_pet_list           = new PetSlot[2];

    [SerializeField] public UIGrid             m_PromoteSkillGrid;
    [SerializeField] public List<SkillSlot>    m_PromoteSkills      = new List<SkillSlot>();


    [Serializable]
    public class PetSlot
    {
        //펫 슬롯
        public UILabel m_label_empty;
        public UISprite m_sprite_pet;
        public UISprite m_sprite_rare;
        public UILabel m_label_level;
        public Sub_GradeMini m_sub_star;
        public UIButtonEx m_button_delete;
        public UIButtonEx m_button_pet_portrait;
        public GameObject m_gameobject_status;
        public UILabel m_label_stat_battle_tier;
        public UILabel m_label_stat_growth_tier;
        public UILabel m_label_stat_product_tier;
        public Animator m_animator_slot;
        public GameObject m_gameobject_sprite;
    }

    [Serializable]
    public class SkillSlot
    {
        public Sub_Skill  m_sub_skill    = null;
        public UIButtonEx m_button_skill = null;
        public Animator   m_Ani_Skill    = null;
        public UILabel    m_Label_Desc   = null;
        public UILabel    m_Label_Name   = null;
    }

    [Serializable]
    public class DragonProductSlot
    {
        public Transform  m_root                       = null;
        public UIButtonEx m_button_dragon_product      = null;
        public UILabel    m_label_dragon_product_count = null;
        public UISprite   m_sprite_draogn_product_icon = null;
        public GameObject   m_effect_socket            = null;
    }


    [Serializable]
    public class Sub_DragonBaseUI
    {
        public UILabel   m_label_dragon_name   = null;
        public UISprite  m_sprite_dragon_class = null;
        public Sub_Grade m_star                = null;

        public UILabel m_label_current_level = null;
        public UILabel m_label_max_level     = null;

        public UILabel       m_label_exp  = null;
        public UIProgressBar m_slider_exp = null;

        public UILabel       m_label_satiety  = null;
        public UIProgressBar m_slider_satiety = null;
        public void Compose_DragonPetBaseUI(long inpetID)
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(inpetID);
            if (petObject == null)
                return;

            var petKindInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_KIND_INFO.Seek_KIND(petObject.m_DragonpetKind);
            if (petKindInfo == null)
                return;

            var petLevelInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_LEVEL_INFO.Seek_KINDLEVEL(petObject.m_DragonpetKind, petObject.m_lv);
            if (null == petLevelInfo)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObject.m_DragonpetKind, petObject.m_grade_lv);
            if (null == petGradeInfo)
                return;
            
            if (null != m_label_dragon_name)
                m_label_dragon_name.Ex_SetText(Util.DragonPet.GetDragonPetNameColored(petObject));

            if (null != m_sprite_dragon_class)
                m_sprite_dragon_class.Ex_SetSpriteByKey(Util.Unit.GetSpriteKey_Superiority_Big(petKindInfo.Class));

            // 성급 (별)
            if (null != m_star)
            {
                m_star.StopAllAnimation();
                m_star.SetGrade(petObject.m_grade_lv);

                if (Util.DragonPet.ValidTryPromote(petObject))
                    m_star.PlayStarLoopAnimation(petObject.m_grade_lv, petObject.m_grade_lv + 1);
            }

            // 레벨
            m_label_current_level.Ex_SetText(petObject.m_lv.ToString());
            m_label_max_level.Ex_SetText(string.Format("/{0}", petGradeInfo.MaxLevel.ToString()));


            //경험치
            if (null != m_label_exp && null != m_slider_exp)
            {
                m_label_exp.Ex_SetActive(petObject.m_lv < petGradeInfo.MaxLevel);
                long req_exp = 0;
                if (EnumCategory.DragonPetExp == (EnumCategory)petLevelInfo.Requirement.GetCategory())
                {
                    if (petLevelInfo.Requirement.KIND == petObject.m_DragonpetKind)
                    {
                        req_exp = petLevelInfo.Requirement.Value1;
                    }
                }

                var cur_exp   = 0f;
                var cur_value = 0f;
                if (0 < req_exp)
                {
                    cur_exp   = petObject.m_exp;
                    cur_value = Mathf.Min(1.0f, (petObject.m_exp / (float)req_exp));
                }

                m_label_exp.Ex_SetText(string.Format("{0}/{1}", Util.ThousandSeparateString(cur_exp), Util.ThousandSeparateString(req_exp)));
                m_slider_exp.value = cur_value;
            }


            int max_satiety = Util.DragonPet.GetMaxFeedingCount(petObject);

            //// 포만감
            if (null != m_label_satiety && null != m_slider_satiety)
            {
                var cur_bar_value = 0f;
                var cur_satiety = 0;

                if (0 < max_satiety)
                {
                    cur_bar_value = Mathf.Min(1.0f, (petObject.m_satiety/ (float)max_satiety));
                    cur_satiety = petObject.m_satiety;
                }

                m_label_satiety.Ex_SetText($"{Util.ThousandSeparateString(cur_satiety)}/{Util.ThousandSeparateString(max_satiety)}");
                m_slider_satiety.value = cur_bar_value;
            }
        }
    }
}


namespace UILogic
{
    // 화면전환, 연출 코드는 GUI_DragonPetInfoScreen_Direct.cs에 작성합니다.
    public partial class GUI_DragonPetInfoScreen : ScreenDisplay<global::GUI_DragonPetInfoScreen>
    {
        public enum EViewState : Int32
        {
            NONE = 0,
            MAIN,             // 메인
            HATCHING,         // 부화
            FORCE_HATCHING,   // 강제 부화 (튜토리얼, 펫 확정뽑기)
            HATCHING_DIRECT,  // 부화 연출
            MULTIPLE_HATCHING_DIRECT, // 다중 부화 연출
            EVOLUTION_DIRECT, // 진화 연출
            PROMOTE,          // 승급
            END
        }
        public enum EPromePetSlot : Int32
        {
            LEFT = 0,
            RIGHT,
            END,
        }

        public enum EHatchRatingButton : Int32
        {
            Normal = 0,
            Unique,
            END,
        }


        //우측 정보창 종류
        public enum EDetailInfoType : Int32
        {
            NONE = 0,
            BASIC,          // 기본
            SKILL,          // 스킬
            FEEDING,        // 먹이주기
            PROMOTE,        // 승급
            HATCHING,       // 부화
            FORCE_HATCHING, // 강제 부화
            END
        }


        //부화 연출 파티클 색깔 바꾸기용
        public class DragonEggBinder
        {
            public List<ParticleSystem> m_particle_systems;
        }

        // 패키지 상품 데이터
        public class GUI_SUMMON_ITEM_DATA : IGUIOnOpenedParam
        {
            public uint m_item_kind;
            public uint m_summon_pet_kind;

            public GUI_SUMMON_ITEM_DATA(uint in_item_kind, uint in_summon_dragon_kind)
            {
                m_item_kind          = in_item_kind;
                m_summon_pet_kind = in_summon_dragon_kind;
            }
        }

        public class GUI_PET_SELECT_DATA : IGUIOnOpenedParam
        {
            public long            m_pet_id;
            public EViewState      m_view_type;
            public EDetailInfoType m_detail_info_type;

            public GUI_PET_SELECT_DATA(long inpetID, EViewState in_view_type = EViewState.MAIN, EDetailInfoType in_detail_info_type = EDetailInfoType.NONE)
            {
                m_pet_id = inpetID;
                m_view_type = in_view_type;
                m_detail_info_type = in_detail_info_type;
            }
        }

        public class GUI_OPEN_BY_FEEDING_ITEM : IGUIOnOpenedParam
        {
            public long            m_dragon_id;
            public GUI_OPEN_BY_FEEDING_ITEM(long inpetID = 0)
            {
                m_dragon_id        = inpetID;
            }
        }

        //additempopup에서 펫 생산품 부족 시 오픈
        public class GUI_OPEN_BY_ADDITEMPOPUP : IGUIOnOpenedParam
        {
        }

        // Stack 에 쌓인 OpenParama 을 제거하기 위해 BackupParam 추가.
        public class UI_STACK_DATA : IGUIOnBackUpParam
        {
            public long m_last_selected_pet_id;
        }

        public override IGUIOnBackUpParam MakeStackData()
        {
            UI_STACK_DATA stack_data = new UI_STACK_DATA();
            stack_data.m_last_selected_pet_id = m_SelectPetID;
            return stack_data;
        }

        class summon_item_info
        {
            public uint m_item_kind          = 0;
            public uint m_summon_pet_kind = 0;
        }

        private List<DragonPetObj.SKILL>                             m_skill_infos                              = new List<DragonPetObj.SKILL>();
        private List<DragonPetObj.SKILL>                             m_PromoteSkill_infos                       = new List<DragonPetObj.SKILL>();
        private Dictionary<long, Sheet_DragonPet.ItemData_DragonPet> m_cashed_pet_items                         = new Dictionary<long, Sheet_DragonPet.ItemData_DragonPet>();
        private List<long>                                           m_sorted_pet_id                            = new List<long>();

        private Dictionary<int /* try_percent */, int /* rate */>    m_promote_probability                      = new Dictionary<int, int>();
        private bool                                                 m_is_update_probability                    = false;
        private Int64                                                m_last_updated_hour                        = 0;

        private List<UInt64>                                         m_product_effect_ids                       = new List<UInt64>();

        private          float                                       m_spin_acc                                 = 0;
        private readonly float                                       m_spin_dampen                              = 2;
        private          BattleBuffStat                              m_old_stat                                 = BattleBuffStat.Empty;

        private bool                                                 m_is_opening_dragon_adventure_by_gui_stack = false;

        // 펫 부화 -> 펫 선택 으로 돌아올때 선택될 펫
        private Int64                                                m_last_selected_pet_id                     = 0;

        private Int64                                                m_SelectPetID                              = 0;
        private Int64                                                m_SelectPromoteSourcePetID                 = 0;
        private Int64                                                m_pet_model_id                             = 0;
        private Int64                                                m_pet_effect_id                            = 0;
        private Int64                                                m_dragon_alter_model_id                    = 0;
        private Int64                                                m_pet_egg_model_id                         = 0;
        private Int64                                                m_dragon_egg_direct_model_id               = 0;                // 연출에 사용되는 알
        private List<Int64>                                          m_pet_egg_model_ids = new List<Int64>(); // 연출에 사용중인 알들 모음, 
        private Animator                                             m_dragon_egg_animator                      = null;

        private Transform                                            m_alter_model_bottom_transform             = null;
        private bool                                                 m_is_drag_init                             = false;
        private float                                                m_dragon_rotation                          = 0;
        private float                                                m_altar_bottom_rotation                    = 0;
        private Coroutine                                            m_co_dragon_easing_stop                    = null;

        private Int32                                                m_select_evolution_step                    = 0;
        private string                                               m_model_name                               = string.Empty;
        private EnumDragonSortType                                   m_pet_sort_type                            = EnumDragonSortType.None;
        private summon_item_info                                     m_summon_item_info                         = null;
        private DragonEggBinder                                      m_dragon_egg_binder                        = null;

        private EDetailInfoType                                      m_current_detail_info_ui;
        private EViewState                                           m_current_view;
        private List<Animator>                                       m_detail_info_ui_animators                 = new List<Animator>();

        private          List<Int32>                                 m_product_slot_count                       = new List<Int32>();
        private          float                                       m_last_product_time                        = 0;
        private readonly float                                       CONST_PRODUCT_BLOCK_TIME                   = 0.25f;

        private bool                                                 m_dont_stop_bgm                            = false;

        private EHatchRatingButton                                   m_Hatch_Current_Type                       = EHatchRatingButton.Normal;


        //public Dictionary<long, EnumDragonState> m_dragon_state_list = new Dictionary<long, EnumDragonState>();

        // 눌렀던 버튼 목록을 [DragonID,수량,버튼Index]로 저장해둡니다.
        // 패킷을 받으면 해당 목록에서 찾아서 로컬을 갱신하고 연출을 재생합니다.

        class DragonProductSlotClicked
        {
            public DragonProductSlotClicked(long inpetID, int in_slot_index, int in_slot_count)
            {
                m_dragon_id  = inpetID;
                m_slot_index = in_slot_index;
                m_slot_count = in_slot_count;
            }

            public long m_dragon_id;
            public int  m_slot_index;
            public int  m_slot_count;
        }


        private List<DragonProductSlotClicked> m_clicked_slot_list = new List<DragonProductSlotClicked>();


        protected override void OnBuilt()
        {
            base.OnBuilt();

            BindControlEvent(OBJ.m_Hatching_RatingButton, OBJ.m_Hatching_RatingButton.onChange, Onclick_HatchRatingButton);
            for (int i = 0; i < OBJ.m_Hatching_RatingButton.itemCount; ++i)
            {
                if (OBJ.m_Hatching_RatingButton[i])
                    OBJ.m_Hatching_RatingButton[i].Identifier = i;
            }

            m_clicked_slot_list.Clear();

            OBJ.m_sheet_pet_list.Initialize(OnSheetEvent_Dragon);
            OBJ.m_sheet_pet_list.RegisterHandlerValidator<Sheet_DragonPet.Block_DragonPet>(Sheet_DragonPet.Block_DragonPet.OBJECTID);
            
            OBJ.m_sheet_feeding.Initialize();
            OBJ.m_sheet_feeding.RegisterHandlerValidator<Sheet_FeedingItem.Block>(Sheet_FeedingItem.Block.OBJECTID);

            OBJ.m_sheet_hatching_pet.Initialize();
            OBJ.m_sheet_hatching_pet.RegisterHandlerValidator<Sheet_HatchingPet.Block>(Sheet_HatchingPet.Block.OBJECTID);

            OBJ.m_sheet_pet_stat.Initialize();
            OBJ.m_sheet_pet_stat.RegisterHandlerValidator<Sheet_DragonPetStat.Block>(Sheet_DragonPetStat.Block.OBJECTID);

            _bind_button_events();
            _bind_init_label();
            _bind_animator();
            _initialize();

#region Inner_Function

            void _bind_button_events()
            {
                BindControlEvent(OBJ.m_button_promote_move         , OBJ.m_button_promote_move.EventOnClick                        , OnClick_OpenPromote);
                BindControlEvent(OBJ.m_button_promote              , OBJ.m_button_promote.EventOnClick                             , OnClickPromote);
                BindControlEvent(OBJ.m_button_back                 , OBJ.m_button_back.EventOnClick                                , OnClick_HardClose);
                BindControlEvent(OBJ.m_button_force_hatching       , OBJ.m_button_force_hatching.EventOnClick                      , OnClickHatching);
                BindControlEvent(OBJ.m_button_hatching             , OBJ.m_button_hatching.EventOnClick                            , OnClickHatching);
                BindControlEvent(OBJ.m_button_hatching_multiple    , OBJ.m_button_hatching_multiple.EventOnClick                   , OnClickMultipleHatching);
                BindControlEvent(OBJ.m_radio_button_sort_options   , OBJ.m_radio_button_sort_options.EventOnClick                  , OnClickRadioSortPriority);
                BindControlEvent(OBJ.m_button_sort_option          , OBJ.m_button_sort_option.EventOnClick                         , OnClickSortOption);
                BindControlEvent(OBJ.m_button_sort_option_close    , OBJ.m_button_sort_option_close.EventOnClick                   , OnClickSortOption);
                BindControlEvent(OBJ.m_button_hatching_link        , OBJ.m_button_hatching_link.EventOnClick                       , OnClick_Product);
                BindControlEvent(OBJ.m_button_go_combine           , OBJ.m_button_go_combine.EventOnClick                          , OnClickGoFeedCombine);
                BindControlEvent(OBJ.m_button_name_change          , OBJ.m_button_name_change.EventOnClick                         , OnClickNameChange);
                BindControlEvent(OBJ.m_button_feeding              , OBJ.m_button_feeding.EventOnClick                             , OnClick_OpenFeeding);
                BindControlEvent(OBJ.m_button_feeding_non_training , OBJ.m_button_feeding_non_training.EventOnClick                , OnClick_OpenFeeding);
                BindControlEvent(OBJ.m_button_sympathize_close     , OBJ.m_button_sympathize_close.EventOnClick                    , OnClick_SoftClose);
                BindControlEvent(OBJ.m_button_skill_info_close     , OBJ.m_button_skill_info_close.EventOnClick                    , OnClick_SoftClose);
                BindControlEvent(OBJ.m_button_hatching_close       , OBJ.m_button_hatching_close.EventOnClick                      , OnClick_SoftClose);
                BindControlEvent(OBJ.m_button_train                , OBJ.m_button_train.EventOnClick                               , OnClick_OpenDragonPetTraining);

                BindControlEvent(OBJ.m_button_hatching_select      , OBJ.m_button_hatching_select.EventOnClick                     , OnClick_OpenHatching);
                BindControlEvent(OBJ.m_button_go_dragon_adventure  , OBJ.m_button_go_dragon_adventure.EventOnClick                 , OnClickGoDragonAdventure);

                BindControlEvent(OBJ.m_button_product              , OBJ.m_button_product.EventOnClick                             , OnClick_Product);

                BindControlEvent(OBJ.m_button_evolution            , OBJ.m_button_evolution.EventOnClick                           , OnClickEvolution);
                BindControlEvent(OBJ.m_button_evolution            , OBJ.m_button_evolution_non_training.EventOnClick              , OnClickEvolution);

                //BindControlEvent(OBJ.m_toggle_button_favority    , OBJ.m_toggle_button_favority.EventOnClick                     , OnClickFavorite);
                //BindControlEvent(OBJ.m_toggle_button_notification, OBJ.m_toggle_button_notification.EventOnClick                 , OnClickAlarm);
                BindControlEvent(OBJ.m_toggle_button_lock          , OBJ.m_toggle_button_lock.EventOnClick                         , OnClickLock);

                //BindControlEvent(OBJ.m_button_basic_product_time , OBJ.m_button_basic_product_time.EventOnClick                  , OnClickProductAll);

                //for (int i = 0; i < OBJ.m_skills.Count; ++i)
                //    BindControlEvent<UIButtonEx, int>(OBJ.m_skills[i].m_button_skill, OBJ.m_skills[i].m_button_skill.EventOnClick, (in_btn, in_idx) => { OnClick_OpenSkillInfo(in_btn, in_idx); }, i);

                //for (int i = 0; i < OBJ.m_draogn_product_slots.Length; ++i)
                //    BindControlEvent<UIButtonEx, int>(OBJ.m_draogn_product_slots[i].m_button_dragon_product, OBJ.m_draogn_product_slots[i].m_button_dragon_product.EventOnClick, (in_btn, in_idx) => { OnClickProductHarvest(in_btn, in_idx); }, i);

                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[0], OBJ.m_button_growth_step[0].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 0); });
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[1], OBJ.m_button_growth_step[1].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 1); });
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[2], OBJ.m_button_growth_step[2].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 2); });
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[3], OBJ.m_button_growth_step[3].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 3); });
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[4], OBJ.m_button_growth_step[4].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 4); });
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[5], OBJ.m_button_growth_step[5].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 5); });            
                //BindControlEvent<UIButtonBase, int>(OBJ.m_button_growth_step[6], OBJ.m_button_growth_step[6].EventOnClick, (in_btn, in_param) => { OnClickGrowthStep(in_btn, 6); });            

                BindControlEvent(OBJ.m_button_probability, OBJ.m_button_probability.EventOnClick, OnClick_ShowProbability);

                BindControlEvent(OBJ.m_pet_list[(int)EPromePetSlot.RIGHT].m_button_delete, OBJ.m_pet_list[(int)EPromePetSlot.RIGHT].m_button_delete.EventOnClick, OnClick_PromoteDeleteRight);
                BindControlEvent(OBJ.m_pet_list[(int)EPromePetSlot.RIGHT].m_button_pet_portrait, OBJ.m_pet_list[(int)EPromePetSlot.RIGHT].m_button_pet_portrait.EventOnClick, OnClick_PromoteRight);


                BindControlEvent(OBJ.m_Button_Info, OBJ.m_Button_Info.EventOnClick, OnClick_Info);

                BindControlEvent(OBJ.m_Button_EquipHero, OBJ.m_Button_EquipHero.EventOnClick, OnClick_EquipHero);

                BindControlEvent(OBJ.m_Button_BattlePower, OBJ.m_Button_BattlePower.EventOnClick, OnClick_BattlePower);



                //for(EPromePetSlot i = EPromePetSlot.LEFT; i < EPromePetSlot.END; ++i)
                //    BindControlEvent<UIXRadioButton, int>(OBJ.m_Hatching_RatingButton[(int)i], OBJ.m_Hatching_RatingButton[(int)i].EventOnClick, (in_btn, in_idx) => { Onclick_HatchRatingButton(in_btn, in_idx); }, (int)i);

            }

            void _bind_init_label()
            {
                OBJ.m_label_hatchable.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HATCHING"));
                OBJ.m_label_sort_name_level.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_LEVEL"));
                OBJ.m_label_sort_name_atk.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_ATTCK"));
                OBJ.m_label_sort_name_def.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_DEFENCE"));
                OBJ.m_label_sort_name_hp.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HP"));

                OBJ.m_label_dragon_feeding_title.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_FEEDING_MENU"));
                OBJ.m_label_sympathize_info_item_empty.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_NON_FOOD"));

                OBJ.m_label_evolution_btn.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_EVOLUTION"));
                OBJ.m_label_evolution_btn_non_training.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_EVOLUTION"));

                OBJ.m_label_feeding_btn.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_FEEDING_MENU"));
                OBJ.m_label_feeding_btn_non_training.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_FEEDING_MENU"));

                OBJ.m_label_force_hatching.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HATCHING"));
                OBJ.m_label_hatching_btn_title.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HATCHING"));
                OBJ.m_label_multiple_hatching_btn_title.Ex_SetText(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HATCHING")); // ********** 텍키 수정 필요 ************ 


                OBJ.m_label_hatch_require_count.Ex_SetText(COMMON_DRAGON_INFO.DRAGON_HATCHING_COUNT); 
                OBJ.m_label_multiple_hatch_require_count.Ex_SetText(COMMON_DRAGON_INFO.DRAGON_HATCHING_COUNT * COMMON_DRAGON_INFO.DRAGON_MUITIPLE_HATCH_COUNT); 


                OBJ.m_label_hatching_able_dragon.Ex_SetText(COMMON_TEXT.GetDragonText("UI_PET_ALTAR_OBTAINABLE"));
                OBJ.m_label_hatching_gain_nest.Ex_SetText(COMMON_TEXT.GetDragonText("UI_PET_ALTAR_GAIN_LOCATION"));
            }


            void _bind_animator()
            {
                m_detail_info_ui_animators.Capacity = (int)EDetailInfoType.END;

                for (int i = 0; i < (int)EDetailInfoType.END; ++i)
                    m_detail_info_ui_animators.Add(null);

                m_detail_info_ui_animators[(int)EDetailInfoType.BASIC]    = OBJ.m_animator_basic;
                m_detail_info_ui_animators[(int)EDetailInfoType.SKILL]    = OBJ.m_animator_skill_info;
                m_detail_info_ui_animators[(int)EDetailInfoType.HATCHING] = OBJ.m_animator_hatching;
                m_detail_info_ui_animators[(int)EDetailInfoType.PROMOTE]  = OBJ.m_animator_promote;
                m_detail_info_ui_animators[(int)EDetailInfoType.FEEDING]  = OBJ.m_animator_feeding;
            }

            void _initialize()
            {
                m_pet_model_id        = 0;
                m_pet_effect_id       = 0;
                m_current_detail_info_ui = EDetailInfoType.NONE;

                for (int i = 0; i < (int)EDetailInfoType.END; ++i)
                {
                    m_detail_info_ui_animators?[i]?.gameObject.SetActive(true);
                    m_detail_info_ui_animators?[i]?.gameObject.SetActive(false);
                }

                for (int i = 0; i < OBJ.m_animator_stat_up.Length; ++i)
                    OBJ.m_animator_stat_up[i].gameObject.SetActive(false);

                //for (int i = 0; i < OBJ.m_draogn_product_slots.Length; ++i)
                //    OBJ.m_draogn_product_slots[i].m_root.Ex_SetActive(false);
            }

#endregion
        }
        protected override void OnOpened(IGUIOnOpenedParam in_open_param)
        {
            base.OnOpened(in_open_param);

            AudioManager.Instance.StopSFX(AudioManager.eSoundTag.TERRITORY);
            AudioManager.Instance.SetVolume_Tag(AudioManager.eSoundTag.TERRITORY, 0);

            UIEventListener.Get(OBJ.m_button_view_drag.gameObject).onDragStart += OnDragStart;
            UIEventListener.Get(OBJ.m_button_view_drag.gameObject).onDrag      += OnDragView;
            UIEventListener.Get(OBJ.m_button_view_drag.gameObject).onDragEnd   += OnDragEnd;

            m_is_update_probability = false;
            m_current_view = EViewState.NONE;
            m_current_detail_info_ui = EDetailInfoType.NONE;
            m_pet_sort_type = EnumDragonSortType.None;
            Select_SortOption(EnumDragonSortType.Level);
            SetPetList();

            OBJ.m_button_force_hatching.Ex_SetActive(false);
            OBJ.m_label_hatching_piece_count_center.Ex_SetActive(false);
            //OBJ.m_toggle_button_notification.Ex_SetActive(false);
            //OBJ.m_toggle_button_favority.Ex_SetActive(false);
            OBJ.m_toggle_button_lock.Ex_SetActive(false);

            m_is_opening_dragon_adventure_by_gui_stack = false;

            OBJ.m_obj_probability.Ex_SetActive(false);

            //SendHandle.GetDragonStateList();

            Int64           selectPetID        = GetFirstDragonPetID();
            EViewState      open_view_type          = EViewState.MAIN;
            EDetailInfoType open_detail_info_type   = EDetailInfoType.NONE;

            if (in_open_param is GUI_SUMMON_ITEM_DATA in_summon_data)
            {
                m_summon_item_info                      = new summon_item_info();
                m_summon_item_info.m_item_kind          = in_summon_data.m_item_kind;
                m_summon_item_info.m_summon_pet_kind = in_summon_data.m_summon_pet_kind;
            }
            else if (in_open_param is GUI_PET_SELECT_DATA in_pet_select_data)
            {
                if(0 != in_pet_select_data.m_pet_id)
                    selectPetID = in_pet_select_data.m_pet_id;

                open_view_type        = in_pet_select_data.m_view_type;
                open_detail_info_type = in_pet_select_data.m_detail_info_type;
            }
            else if (in_open_param is GUI_OPEN_BY_FEEDING_ITEM in_feeding)
            {
                selectPetID = in_feeding.m_dragon_id;
                if(0 == selectPetID)
                    selectPetID = FindHungryDragonID();

                open_detail_info_type = EDetailInfoType.FEEDING;
            }else if (in_open_param is GUI_OPEN_BY_ADDITEMPOPUP)
            {
                selectPetID = FindProductDragonId();
            }
            else if (in_open_param is UI_STACK_DATA in_stack_data)
            {
                selectPetID = in_stack_data.m_last_selected_pet_id;
            }

            // 배경 생성
            CreateBackground();
            Create_AlterModel();
            ModelViewMove(false, true);
            // 타이틀
            UISwitch_Title(true, Single.NegativeInfinity);
            OBJ.m_label_dragon_title.Ex_SetText(COMMON_TEXT.GetDragonText("UI_PET_ALTAR_TITLE"));

            OBJ.m_Hatching_RatingButton.SetCheck((int)EHatchRatingButton.Normal, true);

            // 강제 부화 인지 확인
            if (null != m_summon_item_info || 1 > User.DragonPetContainer.Count)
            {
                OpenView(EViewState.FORCE_HATCHING);
            }
            else if(EViewState.HATCHING == open_view_type)
            {
                OpenView(EViewState.HATCHING);
                UISwitch_Left(false, 1);
                UISwitch_Bottom(false, 1);
            }
            else
            {
                OpenView(EViewState.MAIN);
                UISwitch_Bottom(true, 0);
                UISwitch_Left(true, 0);
                if (false == SelectDragonPet(selectPetID))
                    SelectDragonPet(GetFirstDragonPetID());

                if (open_detail_info_type != EDetailInfoType.NONE)
                    OpenDetailInfoUI(open_detail_info_type, null, true);

                // [튜토리얼-펫 훈련]이 완료되지 않았을경우 펫 모습이 보여지면 안됩니다.
                if (TutorialManager.Instance.IS_RUN_FIRST_DRAGON_TUTORIAL())
                    Release_PetModel();
            }

            // AddItemPopup에서 열었다면 넛지를 추가한다.
            // UI들이 활성화되고 실행되어야 해서 여기에 추가
            if (in_open_param is GUI_OPEN_BY_ADDITEMPOPUP)
                AddProductNudge(m_SelectPetID);

            GlobalSoundCtrl.Instance.Setting_BGM("BGM/BGM_dragon", 2.0f, true);
            GlobalSoundCtrl.Instance.Setting_AMB(string.Empty, 0.5f, true);

            NGraphic.Instance.SetSceneState(NGraphicEnums.EnumSceneState.DragonInfo);

            Util.Grapic.UpdatePowerSaveOption();
        }

        protected override void OnLoop(float _delta)
        {
            base.OnLoop(_delta);

            // 펫 회전 가속도 감속 적용
            if (1 < Mathf.Abs(m_spin_acc))
                m_spin_acc /= m_spin_dampen; // m_spin_acc / 2
            else
                m_spin_acc = 0;

            LoopTTo();

            if (m_is_update_probability)
                LoopProbability();
        }

        protected void LoopTTo()
        {
            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            // 먹이주기 타임태스크 시간 확인
            var tto_feeding = Util.DragonPet.GetDragonPetFeedingTimeTask(petObj.m_DragonPetID);
            OBJ.m_label_basic_feeding_remain_time.Ex_SetText(Util.GetRemainTimeFormat(null != tto_feeding ? tto_feeding.GetRemainTime() : 0));

        }


        protected void LoopProbability()
        {
            long current_hour = Util.GetUTCTime() % Defines.SECONDS_PER_DAY / Defines.SECONDS_PER_HOUR;
            if (current_hour != m_last_updated_hour)
            {
                // 서버에서 갱신이 완료되고 받아와야하기 때문에 약간의 지연시간을 줍니다.
                long second = Util.GetUTCTime() % Defines.SECONDS_PER_MINUTE;
                if (0 < second)
                {
                    OnSendHandle.SendToServer_GS_PROBABILITY_REQ(EnumProbabilityType.DRAGON_PROMOTE);
                    m_last_updated_hour = current_hour;
                }
            }
        }

        protected override void OnClosed(bool in_forced)
        {
            base.OnClosed(in_forced);
            Stop_DragonPetSound();
            AudioManager.Instance.SetVolume_Tag(AudioManager.eSoundTag.TERRITORY, 1);
            OnClose_View(m_current_view, EViewState.NONE);
            OnClose_DetailInfo(m_current_detail_info_ui);

            ReleaseAllModel();

            if (null != m_hatching_camera)
            {
                UnityEngine.Object.Destroy(m_hatching_camera);
                m_hatching_camera = null;
            }


            ClearPetList();
            m_SelectPetID = 0;
            m_SelectPromoteSourcePetID = 0;
            m_skill_infos.Clear();
            m_PromoteSkill_infos.Clear();
            

            ClearEffect();
            ClearProductEffect();
            ClearCoroutine();

            // GUI가 Close 될때만 clear하는 코루틴이라서 이쪽으로 뺏음
            if (null != m_coroutine_stat_up)
                StopCoroutine(m_coroutine_stat_up);
            m_coroutine_stat_up = null;

            if (null != m_coroutine_left_active)
                StopCoroutine(m_coroutine_left_active);
            m_coroutine_left_active = null;

            if (m_coroutine_hatching_skill != null)
                StopCoroutine(m_coroutine_hatching_skill);
            m_coroutine_hatching_skill = null;

            // 모델에 물려있는 코루틴을 제거합니다.
            UIModelManager.Instance.StopAllCoroutines();

            SubSequentJob_BattlePowerUp.UnBlock(SubSequentJob_BattlePowerUp.Reason.PET_STATUS_UP);

            if (null != m_co_dragon_easing_stop)
            {
                StopCoroutine(m_co_dragon_easing_stop);
                m_co_dragon_easing_stop = null;
            }

            UIModelManager.Instance.UI_BACK.model_camera.Ex_SetActive(true);

            // 배경 초기화
            UIModelManager.Instance.UI_BACK.ClearBackOption();
            Util.Grapic.SetCurrentSceneStateGraphic();

            if (false == Util.Dragon.IsOpeningDragonTypeGUI())
            {
                GlobalSoundCtrl.Instance.SettingStage_Default_BGM(2.0f);
                GlobalSoundCtrl.Instance.SettingStage_Default_ABM();

                _try_adventure_dargon_fly();
            }

            m_summon_item_info = null;

            void _try_adventure_dargon_fly()
            {
                var deployer = TerritoryManager.Instance.GetMyTerritoryDeployer();
                if (null == deployer)
                    return;

                ViewTerritoryObject find_building = deployer.FindTerritoryObject((v) =>
                                                                                 {
                                                                                     return v.m_building_type == EnumBuildingKIND.DRAGON_ALTER;
                                                                                 });

                ViewTB_DragonNest dragon_nest_tb = find_building as ViewTB_DragonNest;

                if (null != dragon_nest_tb)
                    dragon_nest_tb.DragonFlyOut();
            }
        }


        // 펫 모록 시트에서 펫 선택
        // * 현재 열려있는 창이 부화창일경우 -> 메인창으로 전환해준다.
        private bool SelectDragonPet(long inpetID, bool in_is_focus = true)
        {
            var petObj = User.DragonPetContainer.GetDragonPetObject(inpetID);
            if (petObj == null)
                return false;

            if (m_SelectPetID != inpetID)
            {
                m_old_stat             = Util.DragonPet.GetBasicStat(petObj);
                m_SelectPetID          = inpetID;
                m_last_selected_pet_id = inpetID;

                // 연출진행중에는 이펙트/사운드를 Clear 처리하지 않는다.
                if (m_current_view != EViewState.EVOLUTION_DIRECT && m_current_view != EViewState.HATCHING_DIRECT)
                {
                    ClearCoroutine();
                    ClearEffect();
                    ClearSFX_Feed();
                    Stop_DragonPetSound();
                }

                switch (m_current_view)
                {
                    case EViewState.NONE:
                    case EViewState.MAIN:
                        if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                            OpenDetailInfoUI(EDetailInfoType.BASIC);
                        break;
                    case EViewState.HATCHING:
                    case EViewState.PROMOTE:
                        OpenView(EViewState.MAIN);
                        break;
                    default:
                        break;
                }

                ComposeDetailInfo_Basic();
                //ComposeProductInfo();
                //ComposeEvolutionStepBack(petObj);
                ChangePetModel(petObj);
                ComposePetAlarm(petObj);

            }

            if (m_cashed_pet_items.TryGetValue(inpetID, out Sheet_DragonPet.ItemData_DragonPet item))
            {
                if (OBJ.m_sheet_pet_list.selectedItem != item.item)
                {
                    OBJ.m_sheet_pet_list.SelectItem(item.item);
                    if (in_is_focus)
                    {
                        var index = OBJ.m_sheet_pet_list.FindItemIndexByData<Sheet_DragonPet.ItemData_DragonPet>(x => x.m_pet_id == m_SelectPetID);

                        OBJ.m_sheet_pet_list.SetViewPositionToItemAt(index);

                        //OBJ.m_sheet_pet_list.SetViewPositionToItem(item.item, true, false);
                    }

                }
            }

            return true;
        }

        // 시트상에서 선택을 해제한다.
        // 기존 펫 모델을 파괴한다.
        private void UnSelectDragon()
        {
            OBJ.m_sheet_pet_list.UnselectAllItem();
            m_SelectPetID = 0;
            //ComposeProductInfo();
            Release_PetModel();
        }

        //ComposeDetailInfo는 우측 UI 정보를 갱신해줍니다.

#region ComposeDetailInfo

#region Basic

        private void ComposeDetailInfo_Basic()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (null == petObject)
                return;

            OBJ.m_sub_base_ui_basic.Compose_DragonPetBaseUI(m_SelectPetID);
            OBJ.m_label_grade.Ex_SetText(Util.DragonPet.GetGradeName(petObject.m_rarity - 1, true));
            OBJ.m_Label_BattlePower.Ex_SetText(Util.ThousandSeparateString(Util.DragonPet.GetBattlePower(petObject)));

            ComposeLevelUpButton_Basic();
            ComposeSkill_Basic();
            ComposeStat_Basic();
            ComposePromoteArrow_Basic();
            Compose_EquipHeroInfo();
        }

        private void ComposeLevelUpButton_Basic()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            var petKindInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_KIND_INFO.Seek_KIND(petObject.m_DragonpetKind);
            if (petKindInfo == null)
                return;

            var petLevelInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_LEVEL_INFO.Seek_KINDLEVEL(petObject.m_DragonpetKind, petObject.m_lv);
            if (null == petLevelInfo)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObject.m_DragonpetKind, petObject.m_grade_lv);
            if (null == petGradeInfo)
                return;

            var trainingData = NDT.COMMON_DRAGON_PET_INFO.DRAGONPET_TRAINING_INFO.Seek_DRAGONPETGRADESTAGE(petObject.m_grade_lv, petObject.m_Pet_Training);

            bool is_remain_training = false;
            int  trainingValue = trainingData == null ? 0 : trainingData.TRAINING_VALUE;
            var trainingInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_TRAINING_INFO.RepoDRAGONPETGRADE(petObject.m_grade_lv);
            if (trainingInfo != null)
                is_remain_training = (trainingInfo.Count > petObject.m_Pet_Training);

            bool is_show_training = Util.DragonPet.Verify_PetTrainingButton(petObject);

            //var lst_training_data = NDT.COMMON_DRAGON_TRAINING_INFO.TABLE.RepoDRAGONMATURITY(dragon_object.evolution);
            //bool is_valid_training = is_show_training
            //                      && 0 < dragon_object.satiety
            //                      && dragon_object.level < Util.Dragon.GetDragonMaxLevel(dragon_object)
            //                      && dragon_object.training < lst_training_data.Count; // 훈련횟수 남음

            bool is_valid_training = is_show_training == true && petObject.m_satiety > 0 && trainingValue <= petObject.m_satiety;
            bool is_level_up_able = petObject.m_lv < petGradeInfo.MaxLevel && petObject.m_satiety < Util.DragonPet.GetMaxFeedingCount(petObject);

            var level_up_state = is_level_up_able ? UIButtonColor.State.Normal : UIButtonColor.State.Disabled;
            var training_state = is_valid_training && is_remain_training ? UIButtonColor.State.Normal : UIButtonColor.State.Disabled;

            if (Util.DragonPet.IsMaxLV(petObject) == true)
                training_state = UIButtonColor.State.Disabled;

            OBJ.m_button_go_dragon_adventure.Ex_SetActive(true); // 모험 버튼은 항상 활성화 되어야 합니다.
            if (petKindInfo.IsTraining == false)
                is_show_training = false;

            OBJ.m_root_button_group_train.Ex_SetActive(is_show_training);
            OBJ.m_root_button_group_non_trainin.Ex_SetActive(!is_show_training);

            if (is_show_training)
            {
                // 훈련 버튼 그룹
                OBJ.m_button_evolution.Ex_SetActive(false);
                OBJ.m_button_feeding.Ex_SetActive(true);
                OBJ.m_button_feeding.SetState(level_up_state, true);
                OBJ.m_button_train.SetState(training_state, true);
            }
            else
            {
                // 일반 버튼 그룹
                OBJ.m_button_evolution_non_training.Ex_SetActive(false);
                OBJ.m_button_feeding_non_training.Ex_SetActive(true);
                OBJ.m_button_feeding_non_training.SetState(level_up_state, true);
            }
        }

        private void Compose_Skill(int in_skill_kind)
        {
            // 엑티브 스킬 여부 체크
            var    active_skill_info          = COMMON_DRAGON_INFO.DRAGON_ACTIVE_SKILL_INFO.SeekByKIND(in_skill_kind);
            bool   is_active_skill            = null != active_skill_info;
            string skill_rank_icon_sprite_key = String.Empty;

            // 스킬 아이콘 세팅
            string skill_icon_sprite_key = null != active_skill_info ? active_skill_info.IconName : Util.Dragon.GetSpriteKey_PassiveSkillIcon(in_skill_kind);

            // 랜덤 스킬의 랭크 아이콘 세팅
            skill_rank_icon_sprite_key = Util.Dragon.GetSpriteKey_SkillRankIcon_ByKind(in_skill_kind);
        }


        private void ComposeSkill_Basic()
        {
            // 현재 보유중인 스킬만 표시하고 미보유중인 스킬은 전부 비활성화 시킵니다.
            // 합성 횟수에 따른 스킬 목록 (2023-08-30 기준) - https://intra.ndream.com/Board/Read/10002/954255
            // 0 : 기본 엑티브(버프)
            // 1 : 기본 엑티브(버프)       , 추가 엑티브(공격)       , 데미지 증가 패시브 1단계 (D~S랭크)
            // 2 : 기본 엑티브(버프)(강화) , 추가 엑티브(공격)       , 이동속도 증가 패시브       , 데미지 증가 패시브 2단계 (D~S랭크)   << 배치상으로 랭크 아이콘이 붙어있는 패시브스킬이 가장 뒤쪽으로 가야한다.
            // 3 : 기본 엑티브(버프)(강화) , 추가 엑티브(공격)(강화) , 이동속도 증가 패시브(강화) , 데미지 증가 패시브 3단계 (D~S랭크)

            // 전체 비활성화
            m_skill_infos.Clear();
            foreach (var skill in OBJ.m_skills)
            {
                if (null == skill)
                    return;
                skill.m_sub_skill.Ex_SetActive(false);
            }

            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            foreach (var skill in petObject.m_skill)
            {
                if (COMMON_BUFF_INFO.TABLE.SeekByKIND((uint)skill.kind) == null)
                    continue;

                m_skill_infos.Add(new DragonPetObj.SKILL(skill.kind, skill.level, skill.Value, skill.Grade));
            }

            for (int i = 0; i < m_skill_infos.Count; ++i)
            {
                if (false == OBJ.m_skills.Ex_IsValidIndex(i))
                    break;

                OBJ.m_skills[i].m_sub_skill.Ex_SetActive(true);
                OBJ.m_skills[i].m_sub_skill.Compose_DragonPetSkill(m_skill_infos[i].kind, m_skill_infos[i].level, m_skill_infos[i].Grade);

                string skill_name = String.Empty;
                string skill_desc = String.Empty;

                // 패시브 스킬
                var buffInfo = COMMON_BUFF_INFO.TABLE.Seek_KIND((UInt32)m_skill_infos[i].kind);
                if (null == buffInfo)
                    return;

                skill_name = COMMON_TEXT.GetSkillText(buffInfo.Name);
                var new_skill_obj_id = new ObjID(EnumCategory.IndexBuff, (UInt32)m_skill_infos[i].kind, (UInt64)m_skill_infos[i].level - 1);

                var ndt_buff_data = COMMON_BUFF_INFO.Table.GetNDTBuffData(new_skill_obj_id);
                if (ndt_buff_data == null)
                    return;

                long buff_value = 0;
                string resultValue = string.Empty;

                // 버프는 한개만 존재하여 최초의 것만 반환
                foreach (var buff_property in ndt_buff_data.GetProperties())
                {
                    if (buff_property.Value.Count == 0)
                        break;

                    var buff_obj_id = buff_property.Value[0];
                    buff_value = buff_obj_id.Value2;

                    // ADD 일 경우에는 수치 그대로 반환한다
                    if (EnumBuffOperator.ADD == (EnumBuffOperator)buff_obj_id.Value1)
                        resultValue = buff_value.ToString();
                    else
                        resultValue = Util.BasisPointValueToString((int)buff_value, false);

                    var font_color = Util.Hero.GetSkillTextFontColor(Util.Hero.EnumSkillValueColorType.DARK, EnumBuffRecipient.Enemy == ndt_buff_data.GetRecipient());
                    resultValue = Util.MakeLabelColorBBCodeText(resultValue, font_color);
                }

                skill_desc = string.Format(COMMON_TEXT.GetSkillText(buffInfo.NameDesc), resultValue);

                OBJ.m_skills[i].m_Label_Name.Ex_SetText(skill_name);
                OBJ.m_skills[i].m_Label_Desc.Ex_SetText(skill_desc);
            }

            //그리드로 재정렬한다.
            OBJ.m_grid.Reposition();
        }

        private void Compose_EquipHeroInfo()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (null == petObject)
                return;

            var equipHeroID = petObject.m_HeroID;
            OBJ.m_Sprite_AddHero.Ex_SetActive(equipHeroID <= 0);
            OBJ.m_Sprite_EquipHero.Ex_SetActive(equipHeroID > 0);
            OBJ.m_Sprite_EquipHeroGrade.Ex_SetActive(equipHeroID > 0);

            if (equipHeroID > 0)
            {
                var heroObj = User.UserHeroManager.Seek(equipHeroID);
                if(heroObj != null)
                {
                    var heroKind = heroObj.HeroUnique;
                    OBJ.m_Label_EquipHero.Ex_SetText(string.Format(COMMON_TEXT.GetText("UI_PET_EQUIP_HERO"),Util.Hero.GetName(heroKind)));
                    OBJ.m_Sprite_EquipHero.Ex_SetSpriteByKey(Util.Hero.GetSpriteKey_Portrait(EnumPortraitType.NORMAL, heroKind));
                    OBJ.m_Sprite_EquipHeroGrade.Ex_SetSpriteByKey(Util.Hero.GetIcon_HugRareBG(heroKind));
                }
            }
            else
                OBJ.m_Label_EquipHero.Ex_SetText(COMMON_TEXT.GetText("UI_PET_EQUIP_NONE"));

        }
        private void ComposeStat_Basic()
        {
            OBJ.m_sheet_pet_stat.ClearAllItems();

            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            // 총 스탯을 기반으로 전투 티어를 구한다.
            short battle_tier = Util.DragonPet.GetComputeBattleTier(petObject);

            //// 전투 타입(사나움) 추가
            //{
            //    var item_data = OBJ.m_sheet_pet_stat.AddItem<Sheet_DragonPetStat.ItemData_DragonPetType>(Sheet_DragonPetStat.Block.OBJECTID);
            //    if (null != item_data)
            //    {
            //        item_data.m_dragon_type      = EnumDragonType.BATTLE;
            //        item_data.m_dragon_type_tier = battle_tier;
            //    }
            //}

            // 공방체 추가
            for (int i = 0; i < (int)Util.DragonPet.EDragonPetStat.END; ++i)
            {
                var item_data = OBJ.m_sheet_pet_stat.AddItem<Sheet_DragonPetStat.ItemData_DefulatStat>(Sheet_DragonPetStat.Block.OBJECTID);
                if (null == item_data)
                    continue;

                item_data.m_petStat = (Util.DragonPet.EDragonPetStat)i;
                item_data.m_petID   = m_SelectPetID;
            }

            //// 그 외 타입 추가
            //var dragon_type_list = Util.Dragon.GetDragonTypes(dragon_object, true);
            //foreach (EnumDragonType dragon_type in dragon_type_list)
            //{
            //    var item_data = OBJ.m_sheet_pet_stat.AddItem<Sheet_DragonPetStat.ItemData_DragonPetType>(Sheet_DragonPetStat.Block.OBJECTID);
            //    if(null == item_data)
            //        continue;

            //    item_data.m_dragon_type      = dragon_type;
            //    item_data.m_dragon_type_tier = dragon_object.GetDragonTypeTier(dragon_type);
            //}
            OBJ.m_sheet_pet_stat.UpdateAllItems();
        }

        private void ComposePromoteArrow_Basic()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            if (Util.DragonPet.ValidTryPromote(petObject))
            {
                OBJ.m_button_promote_move.SetState(UIButtonColor.State.Normal, true);
                OBJ.m_tween_promote_arrow.enabled    = true;
                OBJ.m_sprite_promote_arrow.grayscale = false;
            }
            else
            {
                OBJ.m_button_promote_move.SetState(UIButtonColor.State.Disabled, true);
                OBJ.m_tween_promote_arrow.enabled    = false;
                OBJ.m_tween_promote_arrow.value      = OBJ.m_tween_promote_arrow.from;
                OBJ.m_sprite_promote_arrow.grayscale = true;
            }
        }

#endregion

#region Skill

        private void ComposeDetailInfo_Skill(DragonPetObj.SKILL in_skill)
        {
            OBJ.m_pet_skill.Compose_DragonPetSkill(in_skill.kind, in_skill.level, in_skill.Grade);

            string skill_name = String.Empty;
            string skill_desc = String.Empty;

            // 패시브 스킬
            var buffInfo = COMMON_BUFF_INFO.TABLE.Seek_KIND((UInt32)in_skill.kind);
            if (null == buffInfo)
                return;
            
            skill_name = COMMON_TEXT.GetSkillText(buffInfo.Name);

            var new_skill_obj_id = new ObjID(EnumCategory.IndexBuff, (UInt32)in_skill.kind, (UInt64)in_skill.level - 1);

            var ndt_buff_data = COMMON_BUFF_INFO.Table.GetNDTBuffData(new_skill_obj_id);
            if (ndt_buff_data == null)
                return;

            long buff_value = 0;
            string resultValue = string.Empty;

            // 버프는 한개만 존재하여 최초의 것만 반환
            foreach (var buff_property in ndt_buff_data.GetProperties())
            {
                if (buff_property.Value.Count == 0)
                    break;

                var buff_obj_id = buff_property.Value[0];
                buff_value = buff_obj_id.Value2;

                // ADD 일 경우에는 수치 그대로 반환한다
                if (EnumBuffOperator.ADD == (EnumBuffOperator)buff_obj_id.Value1)
                    resultValue = buff_value.ToString();
                else
                    resultValue = Util.BasisPointValueToString((int)buff_value, false);

                var font_color = Util.Hero.GetSkillTextFontColor(Util.Hero.EnumSkillValueColorType.DARK, EnumBuffRecipient.Enemy == ndt_buff_data.GetRecipient());
                resultValue = Util.MakeLabelColorBBCodeText(resultValue, font_color);
            }

            skill_desc = string.Format(COMMON_TEXT.GetSkillText(buffInfo.NameDesc), resultValue);

            OBJ.m_label_skill_info_name.Ex_SetText(skill_name);
            OBJ.m_label_skill_info_desc.Ex_SetText(skill_desc);
        }

#endregion

#region Feeding

        private void ComposeDetailInfo_Feeding()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            var petKindInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_KIND_INFO.Seek_KIND(petObject.m_DragonpetKind);
            if (petKindInfo == null)
                return;

            Compose_FeedItemList();
            OBJ.m_sub_base_ui_feeding.Compose_DragonPetBaseUI(m_SelectPetID);
        }

        private void Compose_FeedItemList()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            var petLevelInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_LEVEL_INFO.Seek_KINDLEVEL(petObject.m_DragonpetKind, petObject.m_lv);
            if (null == petLevelInfo)
                return;

            long req_exp = 0;
            if((EnumCategory)petLevelInfo.Requirement.GetCategory() == EnumCategory.DragonPetExp)
            {
                if (petLevelInfo.Requirement.KIND == petObject.m_DragonpetKind)
                {
                    req_exp = petLevelInfo.Requirement.Value1;
                }
            }
                        
            OBJ.m_sheet_feeding.ClearAllItems();

            //소화제는 같이 써서 이렇게 넣어줌.

#if UNITY_EDITOR
            var digestiveItem = User.UserItemManager.GetObject(COMMON_DRAGON_PET_INFO.Const.CONST_DRAGON_DIGESTIVE_ITEM_KIND);
            if (digestiveItem != null && digestiveItem.Count > 0)
            {
                var item = OBJ.m_sheet_feeding.AddItem<Sheet_FeedingItem.ItemData>();
                if (item != null)
                {
                    item.petID = petObject.m_DragonPetID;
                    item.item_unique = (int)digestiveItem.m_kind;
                    item.item_count = (int)digestiveItem.Count;
                    item.cur_exp = petObject.m_exp;
                    item.next_exp = req_exp;
                    item.m_batch_button_hide_callback = _hide_batch_button;
                }
            }
#endif
            foreach (var item_obj in User.UserItemManager.GetListByItemType(EnumItemType.DRAGONPETFEEDING))
            {
                if (0 < item_obj.Count)
                {
                    var item = OBJ.m_sheet_feeding.AddItem<Sheet_FeedingItem.ItemData>();
                    if (item != null)
                    {
                        item.petID                        = petObject.m_DragonPetID;
                        item.item_unique                  = (int)item_obj.m_kind;
                        item.item_count                   = (int)item_obj.Count;
                        item.cur_exp                      = petObject.m_exp;
                        item.next_exp                     = req_exp;
                        item.m_batch_button_hide_callback = _hide_batch_button;
                    }
                }
            }

            //foreach (var item_obj in User.UserItemManager.GetListByItemType(EnumItemType.DRAGONEXP))
            //{
            //    if (0 < item_obj.Count)
            //    {
            //        var item = OBJ.m_sheet_feeding.AddItem<Sheet_FeedingItem.ItemData>();
            //        if (item != null)
            //        {
            //            item.petID = petObject.m_DragonPetID;
            //            item.item_unique                  = (int)item_obj.m_kind;
            //            item.item_count                   = (int)item_obj.Count;
            //            item.cur_exp                      = petObject.m_exp;
            //            item.next_exp                     = req_exp;
            //            item.m_batch_button_hide_callback = _hide_batch_button;
            //        }
            //    }
            //}

            OBJ.m_sheet_feeding.gameObject.SetActive(1 <= OBJ.m_sheet_feeding.itemCount);

            System.Comparison<UISheet.IItem> compare = (in_item_a, in_item_b) =>
            {
                var item_a = in_item_a.GetData<Sheet_FeedingItem.ItemData>();
                var item_b = in_item_b.GetData<Sheet_FeedingItem.ItemData>();

                var result = 0;

                do
                {
                    if (null == item_a || null == item_b)
                        break;

                    var item_info_a = Util.Item.GetItemData(item_a.item_unique);
                    var item_info_b = Util.Item.GetItemData(item_b.item_unique);

                    if (null == item_info_a || null == item_info_b)
                        break;

                    // 그룹 아이디 오름차순 정렬
                    if (item_info_a.Sorting != item_info_b.Sorting)
                    {
                        result = item_info_a.Sorting.CompareTo(item_info_b.Sorting);
                        break;
                    }

                    // 아이템 아이디 오름차순 정렬
                    if (item_info_a.Rare != item_info_b.Rare)
                    {
                        result = item_info_a.Rare.CompareTo(item_info_b.Rare);
                        break;
                    }

                    result = item_info_a.KIND.CompareTo(item_info_b.KIND);

                } while (false);

                return result;
            };

            OBJ.m_sheet_feeding.SortItems(compare);
            OBJ.m_sheet_feeding.UpdateAllItems();

            OBJ.m_label_sympathize_info_item_empty.Ex_SetActive(0 >= OBJ.m_sheet_feeding.GetItemLineCount());

            void _hide_batch_button(int in_item_kind)
            {
                int index = OBJ.m_sheet_feeding.FindItemIndex(in_item =>
                                                              {
                                                                  if (in_item.data is Sheet_FeedingItem.ItemData item_data)
                                                                      return item_data.item_unique == in_item_kind;
                                                                  else
                                                                      return false;
                                                              });

                OBJ.m_sheet_feeding.UpdateItemAt(index);
            }
        }

#endregion

#region Promote

        private void ComposeDetailInfo_Promote()
        {
            OBJ.m_sub_base_ui_promote.Compose_DragonPetBaseUI(m_SelectPetID);

            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObject.m_DragonpetKind, petObject.m_grade_lv);
            if (petGradeInfo == null)
                return;

            var petKindInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_KIND_INFO.Seek_KIND(petObject.m_DragonpetKind);
            if (petKindInfo == null)
                return;

            OBJ.m_sub_base_ui_basic.Compose_DragonPetBaseUI(m_SelectPetID);

            m_PromoteSkill_infos.Clear();
            foreach (var skill in OBJ.m_PromoteSkills)
            {
                if (null == skill)
                    return;

                skill.m_sub_skill.Ex_SetActive(false);
            }

            foreach (var skill in petObject.m_skill)
            {
                if (COMMON_BUFF_INFO.TABLE.SeekByKIND((uint)skill.kind) == null)
                    continue;

                m_PromoteSkill_infos.Add(new DragonPetObj.SKILL(skill.kind, skill.level, skill.Value, skill.Grade));
            }

            for(int i = 0; i < m_PromoteSkill_infos.Count;++i)
            {
                if (false == OBJ.m_PromoteSkills.Ex_IsValidIndex(i))
                    break;

                OBJ.m_PromoteSkills[i].m_sub_skill.Ex_SetActive(true);
                OBJ.m_PromoteSkills[i].m_sub_skill.Compose_DragonPetSkill(m_PromoteSkill_infos[i].kind, m_PromoteSkill_infos[i].level, m_PromoteSkill_infos[i].Grade);
            }
            OBJ.m_PromoteSkillGrid.Reposition();


            //if (null == m_coroutine_promote)
            //    OBJ.m_pet_skill_promote.Compose_DragoonSkill(evolution_info.ActiveSkillKind[0], Util.Dragon.GetActiveSkillLevel(evolution_info.ActiveSkillKind[0], dragon_object.dragon_unique, dragon_object.grade));

            var maxLevel = 0;

            var next_grade_info = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObject.m_DragonpetKind, petObject.m_grade_lv + 1);
            if (next_grade_info != null)
                maxLevel = next_grade_info.MaxLevel;

            bool is_max_promote = Util.DragonPet.IsMaxPromote(petObject);

            OBJ.m_label_notify_max_promote.Ex_SetActive(is_max_promote);
            OBJ.m_label_promote_current_max_level.Ex_SetText(petGradeInfo.MaxLevel);
            OBJ.m_sprite_promote_next_arrow.Ex_SetActive(!is_max_promote);
            OBJ.m_label_promote_next_max_level.Ex_SetText(is_max_promote ? "" : maxLevel.ToString());

            ComposeTryRate_Promote();
            ComposeButton_Promote();
            ComposeProbability_Promote();
            //ComposeAllPetSlot_Promote();
        }

        private void ComposeAllPetSlot_Promote()
        {
            ComposePetSlot_Promote(EPromePetSlot.LEFT);
            ComposePetSlot_Promote(EPromePetSlot.RIGHT);
        }

        private void ComposePetSlot_Promote(EPromePetSlot _slotIndex)
        {
            var slot = OBJ.m_pet_list[(int)_slotIndex];
            if (slot == null)
                return;

            var petID = _slotIndex == EPromePetSlot.LEFT ? m_SelectPetID : m_SelectPromoteSourcePetID;

            var petObj = User.DragonPetContainer.GetDragonPetObject(petID);
            if (petObj == null)
            {
                slot.m_label_empty.Ex_SetActive(true);

                slot.m_sprite_pet.Ex_SetActive(false);
                //slot.m_gameobject_status.Ex_SetActive(false);
                slot.m_gameobject_sprite.Ex_SetActive(false);
                slot.m_button_delete.Ex_SetActive(false);
            }
            else
            {
                slot.m_button_delete.Ex_SetActive(_slotIndex == EPromePetSlot.RIGHT);

                slot.m_label_empty.Ex_SetActive(false);
                slot.m_sprite_pet.Ex_SetActive(true);
                slot.m_gameobject_sprite.Ex_SetActive(true);

                var petArtInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_ART_INFO.SeekByKIND(petObj.m_DragonpetKind);
                if (petArtInfo == null)
                    return;

                var petKindInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_KIND_INFO.SeekByKIND(petObj.m_DragonpetKind);
                if (petArtInfo == null)
                    return;

                slot.m_sprite_pet.Ex_SetSpriteByKey(petArtInfo.PortraitName);
                slot.m_label_level.Ex_SetText(petObj.m_lv.ToString());
                slot.m_sprite_rare.Ex_SetSpriteByKey(Util.DragonPet.GetSpriteKey_Rare(petObj.m_rarity));
                slot.m_sub_star.SetGrade(petObj.m_grade_lv);
            }


        }
        public void SetPromoteSorucePetID(Int64 _petID)
        {
            m_SelectPromoteSourcePetID = _petID;

            ComposePetSlot_Promote(EPromePetSlot.RIGHT);
        }


        private void ComposeButton_Promote()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            int target_percent = OBJ.m_sub_try_rate.TargetPercent;

            bool is_max_promote = Util.DragonPet.IsMaxPromote(petObject);
            bool is_promote_able = OBJ.m_sub_try_rate.IsValidAllCondition && Util.DragonPet.ValidPromote(petObject);

            UIButtonColor.State gradeup_state = is_promote_able ? UIButtonColor.State.Normal : UIButtonColor.State.Disabled;

            if (is_max_promote)
                OBJ.m_label_promote_button.Ex_SetText(String.Format(COMMON_TEXT.GetText("UI_AWAKE_DRAGON_MAX")));
            else
                OBJ.m_label_promote_button.Ex_SetText(String.Format(COMMON_TEXT.GetText("UI_AWAKE_DRAGON_RATE"), target_percent.ToString()));

            OBJ.m_button_promote.SetState(gradeup_state, true);
        }


        private void ComposeProbability_Promote()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            int target_percent = OBJ.m_sub_try_rate.TargetPercent;
            bool is_show_probability = 100 != target_percent && false == Util.DragonPet.IsMaxPromote(petObject);
            OBJ.m_sub_probability.Ex_SetActive(is_show_probability);
            if (false == is_show_probability)
                return;

            if (m_promote_probability.TryGetValue(target_percent, out int bp_value))
                OBJ.m_sub_probability.SetPercent(bp_value);
            else
                OBJ.m_sub_probability.SetPercentEmpty();

            OBJ.m_sub_probability.SetActiveToolTip(false);
        }

        private void ComposeTryRate_Promote()
        {
            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            bool is_max_promote = Util.DragonPet.IsMaxPromote(petObject);
            OBJ.m_sub_try_rate.SetActive_Sheet_Slider(!is_max_promote);
            if (is_max_promote)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.Seek_KINDGRADE(petObject.m_DragonpetKind, petObject.m_grade_lv);
            if (petGradeInfo == null)
                return;

            var promoteRateInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_PROMOTE_TRY_RATE.SeekByGrade(petObject.m_grade_lv);
            if (promoteRateInfo == null)
                return;

            OBJ.m_sub_try_rate.SetValidPercentList(null != promoteRateInfo ? promoteRateInfo.Percent.ToList() : null);
            OBJ.m_sub_try_rate.SetConsumption(null != petGradeInfo ? petGradeInfo.Requirements.ToList() : null);
            OBJ.m_sub_try_rate.RefreshRequirement();
        }

        #endregion

        #region Hatching

        private void ComposeDetailInfo_Hatching(EHatchRatingButton in_type)
        {
            // key 연대기 챕터
            // value 해당 연대기에 열리는 펫들
            // -> 늦게 열리는 펫이 가장 상위에 배치되어야 합니다.
            //SortedDictionary<int, List<int>> chronicle_dragons = new SortedDictionary<int, List<int>>(() => { });

            OBJ.m_sheet_hatching_pet.ClearAllItems();
            List<ObjValue> petList = new List<ObjValue>();

            var petItemKind = in_type == EHatchRatingButton.Normal ? (uint)COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_NORMALEGG_KIND : (uint)COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_UNIQUEEGG_KIND;
            if (null != OBJ.m_sheet_hatching_pet)
            {
                petList = Util.DragonPet.GetDragonPetListByItemKind(petItemKind);

                foreach(var petInfo in petList)
                {
                    UContentID petContent = new UContentID();
                    petContent.Data = petInfo.ContentID;

                    var hatching_item = OBJ.m_sheet_hatching_pet.AddItem<Sheet_HatchingPet.ItemData>();
                    if (null == hatching_item)
                        continue;

                    hatching_item.petKind = (int)petContent.KIND;
                }

                OBJ.m_sheet_hatching_pet.UpdateAllItems();
                bool is_empty = 0 >= OBJ.m_sheet_hatching_pet.itemCount;
                if (false == is_empty)
                    OBJ.m_sheet_hatching_pet.SetViewPositionToItemAt(0, true);
            }


            int petEgg = User.UserItemManager.GetCountItem(petItemKind);
            OBJ.m_label_hatching_piece_count.Ex_SetText($"{petEgg}/{COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_HATCHING_COUNT.ToString()}");
            OBJ.m_label_hatching_piece_count_center.Ex_SetText($"{COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_EGG")} : {Util.GetLocalizeNumbericString(Math.Max(0, petEgg))}");

            bool canHatch = Util.DragonPet.ValidAddDragonPet();

            if (petEgg > 0)
            {
                OBJ.m_button_hatching.SetState(UIButtonColor.State.Normal, canHatch);
                OBJ.m_button_hatching_multiple.Ex_SetActive(false);

                // OBJ.m_animator_egg.Play("Dragon_Egg_StandBy");
                if (null != m_dragon_egg_animator)
                    m_dragon_egg_animator.Ex_Play("Hatching_Loop");
            }
            else
            {
                OBJ.m_button_hatching.SetState(UIButtonColor.State.Disabled, true);
                OBJ.m_button_hatching_multiple.Ex_SetActive(false);

                // OBJ.m_animator_egg.Play("Dragon_Egg_Normal");
                if (null != m_dragon_egg_animator)
                    m_dragon_egg_animator.Ex_Play("Hatching_Idle");
            }
        }

#endregion

        // 얘만 예외로 중앙의 부화 버튼도 함께 갱신시킵니다.

#region ForceHatching

        private void ComposeDetailInfo_ForceHatching()
        {
            OBJ.m_button_force_hatching.Ex_SetActive(true);
            OBJ.m_label_egg_info.Ex_SetActive(false);

            var itemCount = User.UserItemManager.GetCountItem(COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_NORMALEGG_KIND);
            if (m_summon_item_info != null || itemCount >= COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_HATCHING_COUNT)
            {
                OBJ.m_button_force_hatching.SetBaseEnabled(true);
            }
            else
            {
                OBJ.m_button_force_hatching.SetBaseEnabled(false);
                OBJ.m_label_egg_info.Ex_SetActive(true);
            }
        }

#endregion

#endregion //ComposeDetailInfo

#region LeftUI

        private void SetHatchEggNewMark()
        {
            int normalEggCount = User.UserItemManager.GetCountItem(COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_NORMALEGG_KIND);
            int uniqueEggCount = User.UserItemManager.GetCountItem(COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_UNIQUEEGG_KIND);

            OBJ.m_Sub_NewMark_NormalEgg.SetNewCount(normalEggCount);
            OBJ.m_Sub_NewMark_UniqueEgg.SetNewCount(uniqueEggCount);
        }
        // 펫 목록 위에있는 알 조각 개수
        private void ComposeHatchingButton_MainView()
        {
            int normalEggCount = User.UserItemManager.GetCountItem(COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_NORMALEGG_KIND);
            int uniqueEggCount = User.UserItemManager.GetCountItem(COMMON_DRAGON_PET_INFO.CONST_DRAGONPET_UNIQUEEGG_KIND);

            // 펫 부화 정보
            OBJ.m_label_hatching_count.Ex_SetActive(false);
            OBJ.m_label_hatchable.Ex_SetActive(true);
            OBJ.m_hatch_able_effect.Ex_SetActive((normalEggCount + uniqueEggCount) > 0);

            OBJ.m_sub_hatching_new_mark.SetNewCount(normalEggCount + uniqueEggCount);
        }

        private void ComposePetAlarm(DragonPetObj inPetOb)
        {
            if (null == inPetOb)
                return;

            // 펫 잠금
            OBJ.m_toggle_button_lock.SetCheck(0 < inPetOb.m_locked);
        }

#region DRAGON_LIST

        private void ClearPetList()
        {
            OBJ.m_sheet_pet_list.ClearAllItems();
            m_cashed_pet_items.Clear();
            m_sorted_pet_id.Clear();
        }

        private void SetHatchTap(EHatchRatingButton in_type)
        {
            if (null != OBJ.m_Hatching_RatingButton)
                OBJ.m_Hatching_RatingButton.SetCheck((int)in_type, true);

            m_Hatch_Current_Type = in_type;


            OnOpen_View(m_current_view, EViewState.NONE, null, true);
            ComposeDetailInfo_Hatching(m_Hatch_Current_Type);
        }

        // 펫 시트를 재구성한다.
        // 시트에서 선택이 해제되기 때문에 필요에 따라 SelectDragon을 함께 호출한다.
        private void SetPetList()
        {
            ClearPetList();

            var petList = new List<DragonPetObj>();

            foreach (var userPet in User.DragonPetContainer.Repository)
                petList.Add(userPet.Value);

            // 펫 펫 목록 정렬
            Util.DragonPet.SortDragonPetList(ref petList, m_pet_sort_type);

            foreach (var petObj in petList)
            {
                var pet = OBJ.m_sheet_pet_list.AddItem<Sheet_DragonPet.ItemData_DragonPet>(Sheet_DragonPet.Block_DragonPet.OBJECTID);
                pet.m_pet_id   = petObj.m_DragonPetID;
                pet.m_pet_kind = petObj.m_DragonpetKind;
                m_cashed_pet_items.Add(petObj.m_DragonPetID, pet);
                m_sorted_pet_id.Add(petObj.m_DragonPetID);
            }

            OBJ.m_sheet_pet_list.UpdateAllItems();

            OBJ.m_label_pet_count.Ex_SetText(string.Format(COMMON_TEXT.GetText("UI_DRAGON_HOLDING"), User.DragonPetContainer.Count.ToString(), Util.DragonPet.GetMaxPetCount().ToString()));
        }


        // SetPetList()는 모든 펫을 재정렬하여 속도가 느리다.
        // 변경된 펫만 재정렬시켜 최적화한다.
        private void UpsertDragonPetList(DragonPetObj inChangePet)
        {
            if (null == inChangePet)
                return;

            // 변경된 펫을 정렬 목록에서 빼낸다
            m_sorted_pet_id.Remove(inChangePet.m_DragonPetID);

            // 변경된 펫의 삽입위치를 찾아서 넣는다.
            int idx = _find_dragon_index(inChangePet);

            if (idx == -1)
                m_sorted_pet_id.Add(inChangePet.m_DragonPetID);
            else
                m_sorted_pet_id.Insert(idx, inChangePet.m_DragonPetID);

            OBJ.m_sheet_pet_list.ClearAllItems();
            m_cashed_pet_items.Clear();

            foreach (var petID in m_sorted_pet_id)
            {
                var petObj = User.DragonPetContainer.GetDragonPetObject(petID);
                if (null == petObj)
                    continue;

                var pet = OBJ.m_sheet_pet_list.AddItem<Sheet_DragonPet.ItemData_DragonPet>(Sheet_DragonPet.Block_DragonPet.OBJECTID);
                pet.m_pet_id   = petObj.m_DragonPetID;
                pet.m_pet_kind = petObj.m_DragonpetKind;
                m_cashed_pet_items.Add(petObj.m_DragonPetID, pet);
            }

            OBJ.m_sheet_pet_list.UpdateAllItems();
            OBJ.m_label_pet_count.Ex_SetText(string.Format(COMMON_TEXT.GetText("UI_DRAGON_HOLDING"), User.DragonPetContainer.Count.ToString(), Util.DragonPet.GetMaxPetCount().ToString()));

            int _find_dragon_index(DragonPetObj inPetObj)
            {
                // 이진탐색을 통해 펫이 삽입될 위치를 찾는다. (맨 뒤에 추가해야할경우 -1 반환)
                // * m_sorted_pet_id는 특정한 우선순위를 기준으로 정렬되어있다.
                //   DragonComparer를 통해 우선순위를 비교하여 in_dragon의 삽입위치를 찾아낸다.

                int left   = 0;
                int right  = m_sorted_pet_id.Count - 1;
                int middle = 0;

                int compare_result = 0;
                while (left <= right)
                {
                    middle = (right + left) / 2;
                    var lhs_dragonPet = User.DragonPetContainer.GetDragonPetObject(inPetObj.m_DragonPetID);
                    var rhs_dragonPet = User.DragonPetContainer.GetDragonPetObject(m_sorted_pet_id[middle]);
                    if (null == lhs_dragonPet || null == rhs_dragonPet)
                        return -1;

                    // 우선순위 : 좌측이 클경우 - , 우측이 클경우 +
                    compare_result = Util.DragonPet.DragonPetComparer(lhs_dragonPet, rhs_dragonPet, m_pet_sort_type);

                    if (0 < compare_result)
                        left = middle + 1; // in_dragon의 우선순위가 더 낮을경우
                    else
                        right = middle - 1; // // in_dragon의 우선순위가 더 높을경우
                }

                // middle이 추가해야하는 위치가 된다.
                int result_index = -1;
                if (0 < compare_result)
                    result_index = middle + 1; // 기존에 있는 펫보다 in_dragon의 우선순위가 더 낮을경우 바로 뒤쪽에 넣어준다.
                else
                    result_index = middle;

                return m_sorted_pet_id.Ex_IsValidIndex(result_index) ? result_index : -1;
            }

        }

        private void UpsertDragonPetList(List<Int64> inPetList)
        {
            foreach (var petID in inPetList)
            {
                var petObj = User.DragonPetContainer.GetDragonPetObject(petID);
                if (petObj == null)
                    continue;

                UpsertDragonPetList(petObj);
            }
        }

        
        private Int64 GetFirstDragonPetID()
        {
            for (int i = 0; i < OBJ.m_sheet_pet_list.itemCount; i++)
            {
                if (OBJ.m_sheet_pet_list.TryGetItemData<Sheet_DragonPet.ItemData_DragonPet>(i, out var item))
                {
                    if (0 != item.m_pet_id)
                        return item.m_pet_id;
                }
            }

            return 0;
        }


        private Int64 FindHungryDragonID()
        {
            for (int i = 0; i < OBJ.m_sheet_pet_list.itemCount; i++)
            {
                if (OBJ.m_sheet_pet_list.TryGetItemData<Sheet_DragonPet.ItemData_DragonPet>(i, out var item))
                {
                    var dragon_object = User.DragonPetContainer.GetDragonPetObject(item.m_pet_id);
                    if (false == Util.DragonPet.VaildDragonPetLevelUP(dragon_object))
                        continue;

                    return item.m_pet_id;
                }
            }

            // 만약 먹이줄 수 있는 펫이 없다면 그냥 기본선택펫으로 간다.
            return GetFirstDragonPetID();
        }

        // 공용 팝업에서 비늘아이템으로 진입했을시 타는 함수
        private long FindProductDragonId()
        {
            DragonObj product_type_dragon = null; // 아무 생산형 펫
            DragonObj productable_dragon = null; // 실제로 비늘 생산이 가능한 펫
            for (int i = 0; i < OBJ.m_sheet_pet_list.itemCount; i++)
            {
                if (OBJ.m_sheet_pet_list.TryGetItemData<Sheet_DragonPet.ItemData_DragonPet>(i, out var item))
                {
                    var dragon_object = User.DragonContainer.GetDragonObject(item.m_pet_id);

                    if (false == dragon_object.HasDragonType(EnumDragonType.PRODUCT))
                        continue;

                    // 생산형 펫을 처음 찾았다면 넣어둔다.
                    if (null == product_type_dragon)
                        product_type_dragon = dragon_object;

                    // 실제 생산이 가능한 펫인지 체크한다.
                    var product_reward_info = NDT.COMMON_DRAGON_PRODUCT_INFO.DRAGON_PRODUCT_TYPE_REWARD_INFO.SeekByEVOLUTION(dragon_object.evolution);
                    if (null == product_reward_info)
                        continue;

                    if (ObjID.Empty == product_reward_info.CHARGE_REWARD)
                        continue;

                    productable_dragon = dragon_object;
                    product_type_dragon = dragon_object;
                    break;
                }
            }

            // 생산 가능한 펫이 있다면
            if (null != productable_dragon)
                return productable_dragon.dragon_id;

            // 생산형 펫이 있다면
            if (null != product_type_dragon)
                return product_type_dragon.dragon_id;

            return GetFirstDragonPetID();
        }

        private void AddProductNudge(long inpetID)
        {
            var dragon_object = User.DragonContainer.GetDragonObject(inpetID);
            if (null == dragon_object)
                return;

            if (false == dragon_object.HasDragonType(EnumDragonType.PRODUCT))
            {
                HintManager.Instance.AddNudgeOnButton(OBJ.m_button_hatching_select);
                return;
            }

            // 실제 생산이 가능한 펫인지 체크한다.
            var product_reward_info = NDT.COMMON_DRAGON_PRODUCT_INFO.DRAGON_PRODUCT_TYPE_REWARD_INFO.SeekByEVOLUTION(dragon_object.evolution);
            if (null == product_reward_info)
                return;

            if (ObjID.Empty != product_reward_info.CHARGE_REWARD)
            {
                //HintManager.Instance.AddNudgeOnButton(OBJ.m_button_basic_product_time);
            }
            else 
            {
                bool is_training_visible = Util.Dragon.Verify_DragonTrainingButton(dragon_object);

                UIButtonEx target_button = null;
                if (Util.Dragon.ValidAltarEvolution(dragon_object)) // 진화 필요 확인
                    target_button = OBJ.m_button_evolution;
                else if (false == Util.Dragon.VaildDragonLevelUP(dragon_object)) // 승급 필요 확인
                    target_button = OBJ.m_button_promote_move;
                else
                    target_button = is_training_visible ? OBJ.m_button_feeding : OBJ.m_button_feeding_non_training;

                HintManager.Instance.AddNudgeOnButton(target_button);
            }
        }

        public void OnSheetEvent_Dragon(eSheetEvent      in_sheet_event
                                      , UISheet          in_sheet
                                      , UISheet.ItemData in_sheet_item_data
                                      , UISheetBlock     in_linked_sheet_block
                                      , GameObject       in_invoked_block_child
                                      , int              in_param)
        {
            Sheet_DragonPet.ItemData_DragonPet dragon_data = in_sheet_item_data as Sheet_DragonPet.ItemData_DragonPet;
            if (null != dragon_data)
            {
                switch (in_sheet_event)
                {
                    case eSheetEvent.ItemClicked:
                    {
                        if (EViewState.MAIN == m_current_view || EViewState.HATCHING == m_current_view)
                        {
                            // 다른 펫을 선택했을 경우 연출 정보들을 날린다.
                            if (m_SelectPetID != dragon_data.m_pet_id)
                            {
                                SubSequentJob_BattlePowerUp.UnBlock(SubSequentJob_BattlePowerUp.Reason.PET_STATUS_UP);

                                var job_list = SubSequentJobManager.Instance.GetTypeOfJob<SubSequentJob_BattlePowerUp>();
                                if (0 != job_list.Count)
                                    job_list[0].ResetNoticeDirect();
                            }

                            SelectDragonPet(dragon_data.m_pet_id);
                            AudioManager.Instance.PlaySFX("SFX/UI/sfx_ui_button_card_00");
                        }
                        else
                        {
                            OBJ.m_sheet_pet_list.UnselectItem(dragon_data.item);
                            if (m_cashed_pet_items.TryGetValue(m_SelectPetID, out Sheet_DragonPet.ItemData_DragonPet dragon))
                            {
                                OBJ.m_sheet_pet_list.SelectItem(dragon.item);
                            }
                        }
                    }
                        break;
                }
            }
        }

#endregion // DRAGON_LIST

#region Sort

        protected void OnClickSortOption(UIButtonBase in_button)
        {
            Toggle_SortOption();
        }

        private void OnClickRadioSortPriority(UIXRadioButton in_radio_button)
        {
            if (UIXButton.current != in_radio_button)
                return;

            var current_item = UIXRadioButton.currentItem;
            if (!current_item || !current_item.isChecked)
                return;

            EnumDragonSortType priority = EnumDragonSortType.Level;
            switch (current_item.Identifier)
            {
                case 0:
                    priority = EnumDragonSortType.Level;
                    break;
                case 1:
                    priority = EnumDragonSortType.Attack;
                    break;
                case 2:
                    priority = EnumDragonSortType.Defense;
                    break;
                case 3:
                    priority = EnumDragonSortType.Hp;
                    break;
            }

            Select_SortOption(priority);
        }

        protected void Toggle_SortOption()
        {
            if (false == OBJ.m_radio_button_sort_options.isActiveAndEnabled)
            {
                OBJ.m_sprite_sort_option_arrow.spriteName = "Icon_Sort_Up";
                OBJ.m_radio_button_sort_options.Ex_SetActive(true);
            }
            else
            {
                OBJ.m_sprite_sort_option_arrow.spriteName = "Icon_Sort_Down";
                OBJ.m_radio_button_sort_options.Ex_SetActive(false);
            }
        }

        protected void Toggle_SortOption(bool in_active)
        {
            if (in_active)
            {
                OBJ.m_sprite_sort_option_arrow.spriteName = "Icon_Sort_Up";
                OBJ.m_radio_button_sort_options.Ex_SetActive(true);
            }
            else
            {
                OBJ.m_sprite_sort_option_arrow.spriteName = "Icon_Sort_Down";
                OBJ.m_radio_button_sort_options.Ex_SetActive(false);
            }
        }

        protected void Compose_SortPriorityLabel()
        {
            string localizer_text = string.Empty;
            switch (m_pet_sort_type)
            {
                case EnumDragonSortType.Level:
                    localizer_text = COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_LEVEL");
                    break;
                case EnumDragonSortType.Attack:
                    localizer_text = COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_ATTCK");
                    break;
                case EnumDragonSortType.Defense:
                    localizer_text = COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_DEFENCE");
                    break;
                case EnumDragonSortType.Hp:
                    localizer_text = COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_HP");
                    break;
            }
            OBJ.m_label_sort_option.text = localizer_text;
        }

        protected void Select_SortOption(EnumDragonSortType in_priority, bool _force = false)
        {
            Toggle_SortOption(false);
            if (_force == false)
            {
                if (m_pet_sort_type == in_priority)
                    return;
            }

            int idx = 0;
            switch (in_priority)
            {
                case EnumDragonSortType.Level:   idx = 0; break;
                case EnumDragonSortType.Attack:  idx = 1; break;
                case EnumDragonSortType.Defense: idx = 2; break;
                case EnumDragonSortType.Hp:      idx = 3; break;
            }

            m_pet_sort_type = in_priority;
            OBJ.m_radio_button_sort_options.SetCheckIndex(idx, true);
            Compose_SortPriorityLabel();
            SetPetList();
            SelectDragonPet(m_SelectPetID);
        }

#endregion // Sort

#endregion //LeftUI

#region BottomUI

        private void SelectEvolutionStep(int in_evolution_step)
        {
            m_select_evolution_step = in_evolution_step;
        }


#endregion

        #region DragonDragRotate

        private void OnDragStart(GameObject in_go)
        {
            if (false == IsDragEnable())
                return;

            m_is_drag_init = false;



            if (null != m_co_dragon_easing_stop)
            {
                StopCoroutine(m_co_dragon_easing_stop);
                m_co_dragon_easing_stop = null;
            }
        }

        private void OnDragView(GameObject in_go, Vector2 in_delta)
        {
            if (false == m_is_drag_init)
            {
                m_is_drag_init = true;
                if (null != UICamera.currentTouch)
                    in_delta -= UICamera.currentTouch.delta;
            }

            if (false == IsDragEnable())
                return;

            float drag_acc = in_delta.x;

            // 회전 방향이 변경되었다면 가속도 제거
            if ((0 < drag_acc && 0 > m_spin_acc) || (0 > drag_acc && 0 < m_spin_acc))
                m_spin_acc = 0;

            m_spin_acc += drag_acc;

            Add_DragonRotate(drag_acc * -0.4f);
            Add_AltarBottomRotate(drag_acc * -0.4f);
        }

        private void OnDragEnd(GameObject in_go)
        {
            if (false == IsDragEnable())
                return;

            if (null != m_co_dragon_easing_stop)
            {
                StopCoroutine(m_co_dragon_easing_stop);
                m_co_dragon_easing_stop = null;
            }
            m_co_dragon_easing_stop = StartCoroutine(EasingStop(-m_spin_acc));
        }

        private bool IsDragEnable()
        {
            // 등장, 진화 애니메이션 중에는 회전 불가
            if (UIModelManager.Instance.UI_BACK.IsPlayAnimation(m_pet_model_id, "Intro")
             || UIModelManager.Instance.UI_BACK.IsPlayAnimation(m_pet_model_id, "Evolution"))
                return false;

            // 메인화면 ,승급창 외의 경우에도 회전 불가
            if (m_current_view != EViewState.MAIN
             && m_current_view != EViewState.PROMOTE)
                return false;
            
            return true;
        }

        private IEnumerator EasingStop(float power)
        {
            float current_time                = 0;
            float start_dragon_rotation       = m_dragon_rotation;
            float start_altar_bottom_rotation = m_altar_bottom_rotation;

            float end_time = OBJ.m_stop_easing_curve[OBJ.m_stop_easing_curve.length - 1].time;

            while (current_time < end_time)
            {
                current_time += Time.deltaTime;
                float ratio = OBJ.m_stop_easing_curve.Evaluate(current_time);
                Set_DragonRotate(start_dragon_rotation + (power * ratio));
                Set_AltarBottomRotate(start_altar_bottom_rotation + (power * ratio));
                yield return new WaitForEndOfFrame();
            }

            m_co_dragon_easing_stop = null;
        }


#endregion

        // 다른 UI를 여는 경우

#region OnClick_Open

        private void OnClick_OpenFeeding(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                return;

            var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObject == null)
                return;

            if (Util.DragonPet.IsMaxLV(petObject, true) == true)
                return;

            OpenDetailInfoUI(EDetailInfoType.FEEDING);
        }

        public void OnClick_OpenDragonPetTraining(UIButtonEx in_button)
        {
            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            var trainingInfo = NDT.COMMON_DRAGON_PET_INFO.DRAGONPET_TRAINING_INFO.RepoDRAGONPETGRADE(petObj.m_grade_lv);
            if (trainingInfo == null)
                return;

            if (petObj.m_lv >= Util.DragonPet.GetDragonPetMaxLevel(petObj))
            {
                Util.AddMessage(COMMON_TEXT.GetNotify("E_PET_TRAINING_NO_POSSIBLE_MAX_LEVEL"));
                return;
            }

            if(petObj.m_Pet_Training >= COMMON_DRAGON_PET_INFO.Const.DRAGONPET_MAX_TRAINING_COUNT)
            {
                Util.AddMessage(COMMON_TEXT.GetNotify("E_PET_TRAINING_NOT_ENOUGH_POINT"));
                return;
            }

            var trainingData = NDT.COMMON_DRAGON_PET_INFO.DRAGONPET_TRAINING_INFO.Seek_DRAGONPETGRADESTAGE(petObj.m_grade_lv, petObj.m_Pet_Training);
            if (trainingData == null)
                return;

            if (petObj.m_satiety <= 0 || trainingData.TRAINING_VALUE > petObj.m_satiety)
            {
                Util.AddMessage(COMMON_TEXT.GetNotify("UI_PET_TRAINING_NO_ENOUGHT"));
                return;
            }

            GUIScene.Instance.OpenDisplay<GUI_DragonPetTrainingListScreen>(new GUI_DragonPetTrainingListScreen.GUI_OPEN_DATA(m_SelectPetID));
                
        }

        private void OnClick_Product(UIButtonBase in_button)
        {
            var itemKind = m_Hatch_Current_Type == EHatchRatingButton.Normal ? COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_NORMALEGG_KIND : COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_UNIQUEEGG_KIND;

            var item_obj_id = new ObjID(EnumCategory.Item, itemKind);
            if (false == Util.Item.VerifyGainRequirement(item_obj_id))
            {
                Util.AddMessage(COMMON_TEXT.GetNotify("E_CANT_USE_FIND_YET"));
                return;
            }

            GUIScene.Instance.OpenDisplay<UILogic.GUI_AddItemPopup>(new UILogic.GUI_AddItemPopup.GUI_OPEN_DATA(item_obj_id, 1));
        }


        private void OnClick_OpenHatching(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;

			OpenHatching();

		}

		public void OpenHatching()
		{
			if (false == Util.DragonPet.ValidAddDragonPet())
			{
				ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_POSSECCION_ADDITION_MAX_COUNT"), User.DragonPetContainer.Count.ToString(), Util.DragonPet.GetMaxPetCount().ToString()));
				return;
			}

			OpenView(EViewState.HATCHING);
		}

        private void Onclick_HatchRatingButton(UIXRadioButton in_radio_button)
        {
            if (UIXButton.current != in_radio_button)
                return;

            var curItem = UIXRadioButton.currentItem;
            if (!curItem || !curItem.isChecked)
                return;


            SetHatchTap((EHatchRatingButton)curItem.Identifier);
        }

        private void OnClick_PromoteRight(UIButtonBase in_button)
        {
            GUIScene.Instance.OpenDisplay<UILogic.GUI_ItemCombination_SelectPopup>(new UILogic.GUI_ItemCombination_SelectPopup.GUI_OPEN_DATA(
                new ObjID(EnumCategory.DragonPet, 0, m_SelectPetID, 0)
                , m_SelectPromoteSourcePetID, GUI_ItemCombination_SelectPopup.EViewListType.PROMOTEPET));
        }
        private void OnClick_PromoteDeleteRight(UIButtonBase in_button)
        {
            m_SelectPromoteSourcePetID = 0;
            ComposePetSlot_Promote(EPromePetSlot.RIGHT);

        }
        private void OnClick_OpenPromote(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;

            if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                return;

            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObj.m_DragonpetKind, petObj.m_grade_lv);
            if (petGradeInfo == null)
                return;

            var next_grade_info = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObj.m_DragonpetKind, petObj.m_grade_lv + 1);
            if (null == next_grade_info)
            {
                ToastMessage.Show(COMMON_TEXT.GetText("UI_HERO_AWAKEN_MAXCOUNT"));
                return;
            }

            if (petGradeInfo.MaxLevel > petObj.m_lv)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_UPGRADE_NOT_ENOUGH_LEVEL")));
                return;
            }

            //if (Util.DragonPet.ValidTryPromote(petObj) == false)
            //    return;

            var equipHero = User.UserHeroManager.Seek(petObj.m_HeroID);
            if(equipHero != null && equipHero.SquadID != 0)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_UPGRADE_HERO_EXPEDITION")));
                return;
            }

            OpenView(EViewState.PROMOTE);
        }

        private void OnClick_OpenSkillInfo(UIButtonBase in_button, int in_slot_idx)
        {
            if (m_current_view != EViewState.MAIN)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                return;

            if (m_skill_infos.Ex_IsValidIndex(in_slot_idx))
            {
                OpenDetailInfoUI(EDetailInfoType.SKILL, new DetailInfoParma_Skill(m_skill_infos[in_slot_idx]));
            }
        }

#endregion

#region OnClick_Content

        // BASIC
        private void OnClickNameChange(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                return;

            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            // 이름 변경 팝업
            GUIScene.Instance.OpenDisplay<GUI_NickNameChangeDragonPopup>(new UILogic.GUI_NickNameChangeDragonPopup.GUI_PET_OPEN_DATA(petObj));
        }

        private void OnClickEvolution(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.BASIC)
                return;

            var dragon_obj = User.DragonContainer.GetDragonObject(m_SelectPetID);
            if (null == dragon_obj)
                return;


            if (false == Util.Dragon.ValidAltarEvolution(dragon_obj))
            {
                var next_grade_req_level = 0;

                var next_grade_info = COMMON_DRAGON_INFO.DRAGON_GRADE_INFO.Seek_KINDGRADE(dragon_obj.dragon_unique, dragon_obj.grade + 1);
                if (null != next_grade_info)
                {
                    next_grade_req_level = next_grade_info.MaxLevel;
                }

                // 최대 등급이면서 용의 레벨이 이미 최대
                if (0 == next_grade_req_level || null == next_grade_info)
                    ToastMessage.Show(COMMON_TEXT.GetNotify("E_DRAGON_MAX_LEVEL"));
                else if (dragon_obj.level <= next_grade_req_level)
                    ToastMessage.Show(COMMON_TEXT.GetDragonText("UI_DRAGON_ALTAR_NON_PREFERMENTUP"));
                return;
            }

            var lst_training_data = NDT.COMMON_DRAGON_TRAINING_INFO.TABLE.RepoDRAGONMATURITY(dragon_obj.evolution);
            if (false == LocalData.WarningTrainChance                // 체크박스
             && dragon_obj.training < lst_training_data.Count        // 훈련횟수 남음
             && 0 < dragon_obj.satiety                               // 포만감이 1 이상 있음
             && Util.Dragon.Verify_DragonTrainingButton(dragon_obj)) // 훈련가능 진화단계
            {
                Action action_call_back = () =>
                                          {
                                              SendHandle.DragonEvolution(dragon_obj.dragon_id);
                                          };

                GUIScene.Instance.OpenDisplay<UILogic.GUI_DragonTrainingNoticePopup>(new UILogic.GUI_DragonTrainingNoticePopup.GUI_OPEN_DATA(dragon_obj, action_call_back));
                return;
            }

            SendHandle.DragonEvolution(dragon_obj.dragon_id);
        }

        private void OnClickGoDragonAdventure(UIButtonBase in_button)
        {
            if (Util.Chronicle.GetChronicleChapter() < NDT.COMMON_DRAGON_PET_INFO.CONST_DIMENSION_INDICATOR_OPEN_CHAPTER)
            {
                Util.AddMessage(COMMON_TEXT.GetNotify(PROTOCOL.Error.E_DIMENSION_IS_NOT_OPEN_CHAPTER.Ex_ToString()));
                return;
            }

            if (!Util.Mission.IsClearedMission(NDT.COMMON_DRAGON_ADVENTURE_INFO.CONST_DIMENSION_ENTER_OPEN_MISSION))
            {
                Util.AddMessage(Util.TextFormat(PROTOCOL.Error.E_DIMENSION_IS_NOT_CLEAR_MISSION.Ex_ToString(), Util.Mission.GetMissionDescText(NDT.COMMON_DRAGON_ADVENTURE_INFO.CONST_DIMENSION_ENTER_OPEN_MISSION)));
                return;
            }

            var stack_type = GUIHistoryManager.Instance.PeekStack();
            if (typeof(UILogic.GUI_PetDimensionSupportScreen) == stack_type)
            {
                // [GUI스택 : 정보창 - 모험 / 현재GUI : 정보창]
                // 이 상황에서 다시 모험으로 진입 시 스텍이 무한히 쌓일 수 있다.
                // 때문에 이전 스택 상태로 되돌린다.
                // 단, 펫 모험GUI의 경우 OpenData가 꼭 필요하기 때문에 아래와 같이 처리해야한다.

                // [GUI스택 : 정보창 / 현재GUI : 정보창] 
                GUIHistoryManager.Instance.PopStack();

                // [GUI스택 : 정보창 - 정보창 / 현재GUI : 모험] 
                // 버튼으로 열면 id 지정
                // 아닐경우 id 안넣음
                long selectPetID = m_is_opening_dragon_adventure_by_gui_stack ? 0 : m_last_selected_pet_id;
                GUIScene.Instance.OpenDisplay<UILogic.GUI_PetDimensionSupportScreen>(new UILogic.GUI_PetDimensionSupportScreen.GUI_OPEN_DATA(selectPetID));

                // // [GUI스택 : 정보창 / 현재GUI : 모험]
                stack_type = GUIHistoryManager.Instance.PeekStack();
                if (typeof(UILogic.GUI_DragonPetInfoScreen) == stack_type)
                    GUIHistoryManager.Instance.PopStack();
            }
            else
            {
                GUIScene.Instance.OpenDisplay<UILogic.GUI_PetDimensionSupportScreen>(new UILogic.GUI_PetDimensionSupportScreen.GUI_OPEN_DATA(m_last_selected_pet_id));
            }
        }

        // FEEDING
        private void OnClickGoFeedCombine(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.FEEDING)
                return;

            var target_min_level = NDT.COMMON_FIELD_OBJECT_SEARCH_UI.GetMinLevel_BySearchTab((int)Util.Chronicle.GetChronicleChapter(), EnumFieldSearchTab.COLONY_BOSS);
            var target_level = Math.Max(Util.Mission.GetCurrentFirstKillLevel(EnumMissionCondition.COLONY_BOSS_DESTROY), target_min_level);

            SendHandle.SendToServer_GS_FIELD_OBJECT_SEARCH_REQ
            (
                        EnumFieldSearchType.FieldBoss_Colony
                        , (byte)target_level
                        , 1
                        , in_packet =>
                        {
                            if (in_packet is PROTOCOL.FLATBUFFERS.GS_FIELD_OBJECT_SEARCH_ACK packet)
                            {
                                Close();
                                FieldManager.Instance.GetFieldObjectManager().ImmediateUpsert(packet.FieldObjectInfo.GetValueOrDefault());

                                short cell_x = packet.CellX;
                                short cell_y = packet.CellY;
                                ObjID fldo_id = packet.FldoId;

                                HintManager.Instance.Hint_FO_MoveAndEffect
                                       (
                                           cell_x
                                           , cell_y
                                           , (long)fldo_id.GetDatabaseID()
                                           , NDT.COMMON_MISSION.CONST_HINT_WAIT_TIME
                                           , EnumIndicatorButtonType.NONE
                                           , Util.ScreenBlock.Reason.NONE
                                           , null
                                           , null
                                           , Util.GetRecommand_FO_SeamLevel()
                                           , Util.GetRecommand_FO_ZDT()
                                           //, new Vector3(0, 0, NDT.COMMON_CONST_INFO.CONST_SEARCH_CAM_OFFSET)
                                       );
                                LocalData.SearchTab = (Int32)EnumFieldSearchTab.COLONY_BOSS;
                                Util.Field.SetSearchIndex(0);
                            }
                        }
                    );


        }
        private void OnClickPromote(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.PROMOTE)
                return;

            if (m_current_detail_info_ui != EDetailInfoType.PROMOTE)
                return;

            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            var petGradeInfo = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObj.m_DragonpetKind, petObj.m_grade_lv);
            if (petGradeInfo == null)
                return;

            var next_grade_info = COMMON_DRAGON_PET_INFO.DRAGONPET_GRADE_INFO.SeekByKINDGRADE(petObj.m_DragonpetKind, petObj.m_grade_lv + 1);
            if (null == next_grade_info)
            {
                ToastMessage.Show(COMMON_TEXT.GetText("UI_HERO_AWAKEN_MAXCOUNT"));
                return;
            }

            if (petGradeInfo.MaxLevel > petObj.m_lv)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_UPGRADE_NOT_ENOUGH_LEVEL")));
                return;
            }

            if (Util.DragonPet.ValidTryPromote(petObj) == false)
                return;

            int percent = OBJ.m_sub_try_rate.TargetPercent;

            if (false == OBJ.m_sub_try_rate.IsValidAllCondition)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_INVEN_LACK_ITEM")));
                return;
            }

            var equipHero = User.UserHeroManager.Seek(petObj.m_HeroID);
            if (equipHero != null && equipHero.SquadID != 0)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_UPGRADE_HERO_EXPEDITION")));
                return;
            }

            string msg_box_title = COMMON_TEXT.GetText("UI_AWAKE_PET_POPUP_TITLE");
            string msg_box_desc = string.Format(COMMON_TEXT.GetText("UI_AWAKE_PET_POPUP_DESC"), (petObj.m_grade_lv + 1).ToString(), percent.ToString());

            if (100 == percent)
            {
                SendHandle.PetGradeup(m_SelectPetID, percent);
            }
            else
            {
                if (false == LocalData.WarningPopUpByDragonRatePromote)
                {
                    MessageBox.ShowCb
                        (
                            MessageCheckBoxType.CHECKBOX_TRY_DRAGON_RATE_PROMOTE
                          , msg_box_title
                          , msg_box_desc                                                       // desc
                          , COMMON_TEXT.GetText("UI_POPUP_GO_SHOP_OK")                         // left_button
                          , COMMON_TEXT.GetText("UI_POPUP_GO_SHOP_CANCEL")                     // right_button
                          , COMMON_TEXT.GetText("UI_CHECK_POPUP_NO_ACCEPT")                    // toggle
                          , () => { SendHandle.PetGradeup(m_SelectPetID, percent); } // left_btn_click_callback
                          , null                                                               // right_btn_click_callback
                          , MessageBoxCancelBtnType.RIGHT
                        );
                }
                else
                {
                    SendHandle.PetGradeup(m_SelectPetID, percent);
                }
            }
        }

        // HATCHING
        private void OnClickGoDragonNest(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.HATCHING)
                return;
            if (m_current_detail_info_ui != EDetailInfoType.HATCHING)
                return;

            //펫 둥지로 이동
            Hint_TB_DragonNest.Execute_Hint_TB_DragonNest();
        }

        private void OnClickHatching(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.HATCHING
             && m_current_view != EViewState.FORCE_HATCHING)
                return;

            if (false == Util.DragonPet.ValidAddDragonPet())
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_POSSECCION_ADDITION_MAX_COUNT"), User.DragonPetContainer.Count.ToString(), Util.DragonPet.GetMaxPetCount().ToString()));
                return;
            }

            int item_kind = 0;
            int item_count = 0;
            int hatching_able_count = 0;
            // 몇몇 KIND 에 대해서 처리가 필요하다.
            if (null != m_summon_item_info)
            {
                item_kind = (int)m_summon_item_info.m_item_kind;
                item_count = User.UserItemManager.GetCountItem((uint)m_summon_item_info.m_item_kind);
            }
            else
            {
                item_kind  = m_Hatch_Current_Type == EHatchRatingButton.Normal ? (int)COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_NORMALEGG_KIND : (int)COMMON_DRAGON_PET_INFO.Const.CONST_DRAGONPET_UNIQUEEGG_KIND;
                item_count = User.UserItemManager.GetCountItem((uint)item_kind);
            }

            hatching_able_count = 1;

            if (item_count < hatching_able_count)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_INVEN_LACK_ITEM")));
                return;
            }


            int summon_pet_kind = 0;
            if (null != m_summon_item_info)
                summon_pet_kind = (int)m_summon_item_info.m_summon_pet_kind;

            SendHandle.DragonPetHatching(item_kind, summon_pet_kind);
        }

        private void OnClickMultipleHatching(UIButtonBase in_button)
        {
            if (m_current_view != EViewState.HATCHING
             && m_current_view != EViewState.FORCE_HATCHING)
                return;

            TryMultipleHatching();
        }

        private void TryMultipleHatching()
        {
            if (false == Util.Dragon.ValidAddDragon(COMMON_DRAGON_INFO.DRAGON_MUITIPLE_HATCH_COUNT))
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_PET_POSSECCION_ADDITION_MAX_COUNT"), User.DragonPetContainer.Count.ToString(), Util.Dragon.GetMaxDragonCount().ToString()));
                return;
            }

            var item_kind = COMMON_DRAGON_INFO.DRAGON_RANDOM_PIECE_UNIQUE;

            var item_count = User.UserItemManager.GetCountItem((uint)COMMON_DRAGON_INFO.DRAGON_RANDOM_PIECE_UNIQUE);
            var hatching_able_count = NDT.COMMON_DRAGON_INFO.DRAGON_HATCHING_COUNT * COMMON_DRAGON_INFO.DRAGON_MUITIPLE_HATCH_COUNT;

            if (item_count < hatching_able_count)
            {
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("E_INVEN_LACK_ITEM")));
                return;
            }
            // dragon_kind 가 0 이면 랜덤으로 소환.
            SendHandle.DragonMultipleHatching(item_kind, COMMON_DRAGON_INFO.DRAGON_MUITIPLE_HATCH_COUNT);
        }


        // ETC
        private void OnClickGrowthStep(UIButtonBase in_button, int in_step)
        {
            if (m_current_view != EViewState.MAIN
             && m_current_view != EViewState.PROMOTE)
                return;

            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (null == petObj)
                return;

            if (m_select_evolution_step == in_step)
                return;

            //if (m_dragon_birth_evo > in_step) // 최소 진화단계 제한
            //    return;

            ChangePetModel(petObj.m_DragonpetKind);

            ClearEffect();
            ClearCoroutine();
            ClearSFX_Feed();
            Stop_DragonPetSound();

            Play_DragonPetIntro(petObj.m_DragonpetKind);
        }


        private void OnClick_ShowProbability(UIButtonBase in_button)
        {
            Util.OpenProbabilityWebSite(EnumCategory.DragonPet);
        }

        private void OnClick_BattlePower(UIButtonBase _in_button)
        {
            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            OBJ.m_sub_tool_tip_buff.Ex_SetActive(true);
            OBJ.m_sub_tool_tip_buff.PetCompose(petObj);
        }

        private void OnClick_EquipHero(UIButtonBase _in_button)
        {
            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            if (petObj.m_HeroID <= 0)
            {
                GUIScene.Instance.OpenDisplay<UILogic.GUI_KnightInfoScreen>(null);
                return;
            }

            var hero = User.UserHeroManager.Seek(petObj.m_HeroID);
            if (hero != null)
            {
                GUIScene.Instance.OpenDisplay<UILogic.GUI_KnightInfoScreen>(new UILogic.GUI_KnightInfoScreen.GUI_OPEN_DATA(hero.HeroUnique));
                return;
            }

            GUIScene.Instance.OpenDisplay<UILogic.GUI_KnightInfoScreen>(null);
        }
        private void OnClick_Info(UIButtonBase _in_button)
        {
            string title = string.Empty;
            List<AdviceData> list_advice = new List<AdviceData>();
            var help_default_data = NDT.CLIENT_HELP_POPUP_INFO.DEFAULT.Seek_KIND((UInt16)NDT.CLIENT_HELP_POPUP_INFO.KIND_DRAGONPET);
            if (help_default_data != null)
            {
                title = COMMON_TEXT.GetText(help_default_data.Name);

                var help_table_data = NDT.CLIENT_HELP_POPUP_INFO.TABLE.RepoKIND(help_default_data.KIND);
                if (help_table_data != null)
                {
                    foreach (var data in help_table_data)
                    {
                        AdviceData advice_data = new AdviceData(COMMON_TEXT.GetText(data.Name), COMMON_TEXT.GetText(data.Desc), data.SpriteKey);
                        list_advice.Add(advice_data);
                    }
                }
            }

            MessageBox.ShowAdvice(title, list_advice);

            //GUIScene.Instance.OpenDisplay<UILogic.GUI_OptionBuffInfoPopup>(
            //    new GUI_OptionBuffInfoPopup.GUI_OPEN_DATA_PET(GUI_OptionBuffInfoPopup.EnumKnightEquipTabType.PETSKILL, m_SelectPetID, 1));
            //return;
        }

        //private void OnClickFavorite(UIXToggleButton in_button)
        //{
        //    if (m_current_view != EViewState.MAIN)
        //        return;

        //    OBJ.m_toggle_button_favority.SetCheck(true);

        //    DragonObj dragon_object = User.DragonContainer.GetDragonObject(m_SelectPetID);
        //    if (null == dragon_object)
        //        return;
        //    if (0 == dragon_object.favority)
        //    {
        //        // 즐겨찾기 등록
        //        SendHandle.DragonFavority(m_SelectPetID, 1);
        //        ToastMessage.Show(Util.Text("UI_DRAGON_CHANGE_REPRESENT"));
        //    }
        //    else
        //    {
        //        ToastMessage.Show(Util.Text("UI_DRAGON_ALREADY_REPRESENT"));
        //    }
        //}

        //private void OnClickAlarm(UIXToggleButton in_button)
        //{
        //    if (m_current_view != EViewState.MAIN)
        //        return;

        //    DragonObj dragon_object = User.DragonContainer.GetDragonObject(m_SelectPetID);
        //    if (null == dragon_object)
        //        return;

        //    if (true == User.DragonContainer.GetAlarmDragon(m_SelectPetID))
        //    {
        //        User.DragonContainer.SetAlarmDragon(m_SelectPetID, false);
        //        ToastMessage.Show(Util.Text("UI_DRAGON_STOP_ALARM"));
        //    }
        //    else
        //    {
        //        User.DragonContainer.SetAlarmDragon(m_SelectPetID, true);
        //        ToastMessage.Show(Util.Text("UI_DRAGON_ACTIVE_ALARM"));
        //    }

        //    ComposeDragonAlarm(dragon_object);
        //    if (m_cashed_pet_items.TryGetValue(dragon_object.dragon_id, out Sheet_DragonPet.ItemData_DragonPet item))
        //        OBJ.m_sheet_pet_list.UpdateItem(item.item);
        //    GameEventHandler.Instance.AddDispatchedEvent(DRAGON_CHANGE.ID, DRAGON_CHANGE.CACHED);
        //}

        private void OnClickLock(UIXToggleButton in_button)
        {
            if (m_current_view != EViewState.MAIN)
                return;

            var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
            if (petObj == null)
                return;

            OBJ.m_toggle_button_lock.SetCheck(0 < petObj.m_locked);

            if (0 == petObj.m_locked)
                SendHandle.PetLock(m_SelectPetID, 1);
            else
                SendHandle.PetLock(m_SelectPetID, 0);
        }

        protected override bool ConfirmToClose(UIButtonBase ctrl, bool touched, bool escaped)
        {
            return CloseProcess(!escaped);
        }

        private void OnClick_SoftClose(UIButtonBase in_button)
        {
            if (CloseProcess(false))
                Close();
        }

        private void OnClick_HardClose(UIButtonBase in_button)
        {
            if (CloseProcess(true))
                Close();
        }

        public void CreateNudge_go_dragon_adventure()
        {
            HintManager.Instance.AddNudgeOnButton(OBJ.m_button_go_dragon_adventure);
        }


        //GUI가 파괴되야하는 상황이면 true 리턴
        //그외에 상황에는 false 리턴
        private bool CloseProcess(bool in_is_hard_close = false)
        {
            // 좌측 상단 close 버튼을 눌렀을 때
            if (in_is_hard_close)
            {
                if (EViewState.PROMOTE == m_current_view)
                    OpenView(EViewState.MAIN);
                else
                    return (false == _try_open_gui_from_stack());

                return false;
            }

            switch (m_current_view)
            {
                case EViewState.MAIN:
                    switch (m_current_detail_info_ui)
                    {
                        case EDetailInfoType.BASIC:
                            return (false == _try_open_gui_from_stack());
                        case EDetailInfoType.FEEDING:
                        case EDetailInfoType.SKILL:
                            OpenDetailInfoUI(EDetailInfoType.BASIC);
                            break;
                        default:
                            return (false == _try_open_gui_from_stack());
                    }
                    break;
                case EViewState.PROMOTE:
                    OpenView(EViewState.MAIN);
                    break;
                case EViewState.HATCHING:
                    if (0 == m_last_selected_pet_id)
                        m_last_selected_pet_id = GetFirstDragonPetID();

                    SelectDragonPet(m_last_selected_pet_id);
                    break;
                case EViewState.FORCE_HATCHING:
                    return (false == _try_open_gui_from_stack());
                default:
                    break;
            }

            return false;

            // stack에 있는 GUI를 별도로 열었다면 true 리턴
            bool _try_open_gui_from_stack()
            {
                // 몇몇 GUI는(펫 모험) 응답 패킷을 OpenParam으로 사용합니다.
                // 스택에 이런 GUI가 존재할경우 따로 열어주도록 합니다.
                var stack_type = GUIHistoryManager.Instance.PeekStack();
                if (typeof(UILogic.GUI_DimensionSupportScreen) == stack_type)
                {
                    m_is_opening_dragon_adventure_by_gui_stack = true;
                    SendHandle.GetDragonList();
                    return true;
                }
                else
                {
                    return false;
                }

            }
        }



        #endregion


        // 아이템 개수가 적으면 윗부분이 말려들어가는 이슈가 있어서 따로 관리
        private void SetSheetScrollBottom(UISheet in_sheet)
        {
            if (0 >= in_sheet.itemCount)
                return;

            // 아이템들의 총 높이를 계산하여 어느쪽으로 스크롤할지 결정한다.
            float panel_height = in_sheet.scrollViewPanel.GetViewSize().y;
            int   block_height = in_sheet.itemBlockSize.y;

            int total_height = in_sheet.itemCount * block_height;
            if (panel_height < total_height)
            {
                in_sheet.SetViewPositionToItemAt(in_sheet.itemCount - 1, true);
            }
            else
            {
                in_sheet.SetViewPositionToItemAt(0, true);
            }
        }

        [ReceiveGameEvent(typeof(GS_DAY_NIGHT_NFY_EVENT),
                          typeof(DRAGON_MULTIPLE_HATCH_POPUP_CLOSE_SELF),
                          typeof(GET_PVP_DECK),
                          typeof(LORD_SUPPORTED_BUFF_CHANGE))]
        protected override void OnUpdateGameEvent(HashSet<Type> in_event_ids)
        {
            if (in_event_ids.Contains(typeof(GS_DAY_NIGHT_NFY_EVENT)))
            {
                if (m_current_view != EViewState.PROMOTE)
                {
                    string show_sprite_key = GetBackgroundSpriteKey(false /* is_promote */, LocalData.IsDay);
                    string hide_sprite_key = GetBackgroundSpriteKey(false /* is_promote */, !LocalData.IsDay);
                    ChangeBackground(show_sprite_key, hide_sprite_key);
                }
            }
            else if (in_event_ids.Contains(typeof(DRAGON_MULTIPLE_HATCH_POPUP_CLOSE_SELF)))
            {
                ModelViewMove(false, false);

                if (0 == m_last_selected_pet_id)
                    m_last_selected_pet_id = GetFirstDragonPetID();

                SelectDragonPet(m_last_selected_pet_id);

                OpenView(EViewState.MAIN);
            }
            
            if (in_event_ids.Contains(typeof(LORD_SUPPORTED_BUFF_CHANGE)))
            {
                if (EViewState.MAIN == m_current_view && EDetailInfoType.FEEDING == m_current_detail_info_ui)
                {
                    OBJ.m_sheet_feeding.UpdateItemsInViewport();
                }
            }
            //else if (in_event_ids.Contains(typeof(GET_PVP_DECK)))
            //{
            //    var mainDragon = User.DragonContainer.GetDragonObject(m_SelectPetID);
            //    if (null == mainDragon)
            //        return;

            //    var subDragon = User.DragonContainer.GetDragonObject(m_Select_SubDragon_Id);
            //    if (subDragon == null)
            //        return;

            //    var pvpDeck = Util.Dragon.RemoveDragonFromArenaDeck(m_Select_SubDragon_Id);
            //    if (pvpDeck != null)
            //        SendHandle.SendToServer_GS_GLOBALPVP_DECK_SET_REQ(pvpDeck);

            //    SendHandle.SubDragonAdd(mainDragon.dragon_id, subDragon.dragon_id, m_Select_SubDragon_SlotIndex);
            //    m_Select_SubDragon_SlotIndex = 0;
            //    m_Select_SubDragon_Id = 0;
            //}

        }

        [ReceiveGameEvent(typeof(DRAGON_LEVELUP),
                          typeof(PET_GRADEUP),
                          typeof(DRAGON_EVOLUTION),
                          typeof(PET_FEEDING),
                          typeof(DRAGONPET_HATCHING),
                          typeof(DRAGON_MULTIPLE_HATCHING),
                          typeof(DRAGONPET_CHANGE),
                          typeof(PET_LOCK),
                          typeof(ITEM_CHANGE),
                          typeof(TTo_DragonProductCharge),
                          typeof(EVENT_ON_RECONNECT),
                          typeof(DRAGON_HARVEST),
                          typeof(DRAGON_HARVEST_ALL),
                          typeof(PROBABILITY_PARAM),
                          typeof(DRAGON_MULTIPLE_HATCH_RETRY),
                          typeof(SUBDRAGON_UPDATE),
                          typeof(GET_DRAGON_STATE_LIST))]

        protected override void OnDispatchGameEvent(Type in_event_id, IEventDispatchParam in_param)
        {
            if (in_param is DRAGON_LEVELUP param_level_up)
            {
                var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
                if (petObj == null)
                    return;


                //if (m_select_evolution_step != dragon_object.evolution)
                //{

                //}
                ChangePetModel(petObj);

                Refresh_CurrentDetailInfo();

                if (param_level_up.m_is_levelup)
                {
                    PlayDirect_LevelUp(petObj);
                    SetPetList();
                }

                AudioManager.Instance.PlaySFX("SFX/UI/sfx_ui_exp_use");
                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("PET_EXP_ACQUISITION")
                                              , Util.ThousandSeparateString(param_level_up.m_gain_exp)));
            }
            else if (in_param is PET_GRADEUP param_gradeup)
            {
                var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
                if (petObj == null)
                    return;

                SetPetList();
                SelectDragonPet(m_SelectPetID);

                Refresh_CurrentDetailInfo();
                PlayDirect_Promote(petObj, param_gradeup.m_is_success);
            }
            else if (in_param is DRAGON_EVOLUTION)
            {
                var dragon_object = User.DragonContainer.GetDragonObject(m_SelectPetID);
                if (null == dragon_object)
                    return;

                // 연출과정에서 펫 목록 시트를 갱신해 줍니다.
                OpenView(EViewState.EVOLUTION_DIRECT);
            }
            else if (in_param is PET_FEEDING param_feeding)
            {
                var petObj = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
                if (petObj == null)
                    return;

                Refresh_CurrentDetailInfo();
                PlayDirect_Feeding(param_feeding);
                if (Util.DragonPet.IsMaxLV(petObj))
                    OpenDetailInfoUI(EDetailInfoType.BASIC);

                ToastMessage.Show(string.Format(COMMON_TEXT.GetNotify("PET_EXP_ACQUISITION")
                                              , Util.ThousandSeparateString(param_feeding.m_gain_exp)));

                OBJ.m_animator_exp_gauge.Ex_SetActive(false);
                OBJ.m_animator_exp_gauge.Ex_SetActive(true);


                if (param_feeding.m_is_levelup)
                {
                    UpsertDragonPetList(petObj); // / 레벨업으로 인한 해당 펫 재정렬
                    SelectDragonPet(m_SelectPetID);
                }
                else
                {
                    OBJ.m_sheet_pet_list.UpdateItemsInViewport(); // 배고픔 노티 없애기 위해 Update
                }
            }
            else if (in_param is DRAGONPET_HATCHING param_hatching)
            {
                bool is_success = true;
                do
                {
                    m_summon_item_info = null;

                    var petObj = User.DragonPetContainer.GetDragonPetObject(param_hatching.m_petID);
                    BindEggModel(petObj);

                    OpenView(EViewState.HATCHING_DIRECT, new ViewStateParma_Hatching(param_hatching.m_petID));
                } while (false);
            }
            else if (in_param is DRAGON_MULTIPLE_HATCHING param_multiple_hatching)
            {

                for (int i = 0; i < param_multiple_hatching.m_dragon_id_list.Count; ++i)
                {
                    var dragon_object = User.DragonContainer.GetDragonObject(param_multiple_hatching.m_dragon_id_list[i]);
                    if (null == dragon_object)
                        return;
                }


                if (EViewState.MULTIPLE_HATCHING_DIRECT == m_current_view && GUIScene.Instance.IsDisplayOpened<GUI_DragonHatchResultPopup>())
                {
                    UpsertDragonPetList(param_multiple_hatching.m_dragon_id_list);
                    return;
                }

                if (false == BindEggModel(null))
                    return;

                OpenView(EViewState.MULTIPLE_HATCHING_DIRECT, new ViewStateParma_Multiple_Hatching(param_multiple_hatching.m_dragon_id_list));
            }
            else if (in_param is DRAGONPET_CHANGE)
            {
                var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
                if (null == petObject)
                    return;

                ComposePetAlarm(petObject);
                Refresh_CurrentDetailInfo();
            }
            else if (in_param is ITEM_CHANGE)
            {
                ComposeHatchingButton_MainView();
                Refresh_CurrentDetailInfo();
            }
            //else if (in_param is TIME_TASK_FINISH tto_finish)
            //{
            //    var tto_dragon_product = Util.Dragon.GetDragonProductTimeTask(m_SelectPetID);
            //    if (null == tto_dragon_product)
            //        return;

            //    if (tto_dragon_product.TargetObjID.DatabaseID == tto_finish.Target_ID.DatabaseID)
            //        ComposeProductInfo();
            //}
            else if (in_param is EVENT_ON_RECONNECT event_param)
            {
                GUIHistoryManager.Instance.ClearStack();
                GUIScene.Instance.CloseAllWindowExceptMainScreenWindows();
                GUIScene.Instance.MainUIOpen();
            }
            else if (in_param is DRAGON_HARVEST dragon_harvest)
            {
                int cliked_list_index = -1;
                for (int i = 0; i < m_clicked_slot_list.Count; ++i)
                {
                    if (m_clicked_slot_list[i].m_dragon_id == dragon_harvest.m_dragon_id
                     && m_clicked_slot_list[i].m_slot_count == dragon_harvest.m_item_count)
                    {
                        cliked_list_index = i;
                        break;
                    }
                }
                if (cliked_list_index == -1)
                    return;

                // 어떤 슬롯을 눌렀는지 가져온다.
                int slot_index = m_clicked_slot_list[cliked_list_index].m_slot_index;

                // 로컬 정보를 갱신해준다.
                int remain_count = 0;
                AddLocalSlot(dragon_harvest.m_dragon_id, slot_index, -(int)dragon_harvest.m_item_count, ref remain_count);
                m_product_slot_count[slot_index] = remain_count;

                // 현재 선택중인 펫이라면 연출을 재생한다.
                //if (dragon_harvest.m_dragon_id == m_SelectPetID)
                //{
                //    GUI_ItemDirector.AddItemDirect_UI(OBJ.m_draogn_product_slots[slot_index].m_root.transform, (int)dragon_harvest.m_item_kind);
                //    EffectManager.Instance.PlayEffectOnce(OBJ.m_draogn_product_slots[slot_index].m_root.gameObject, "FX_UI_TOUCH", Vector3.zero, Vector3.one, 0);
                //    AudioManager.Instance.PlaySFX("SFX/UI/sfx_ui_get_meat");
                //    OBJ.m_draogn_product_slots[slot_index].m_root.Ex_SetActive(0 < remain_count);
                //}

                // 클릭한 버튼 리스트에서 제거해준다.
                m_clicked_slot_list.RemoveAt(cliked_list_index);
            }
            else if (in_param is DRAGON_HARVEST_ALL dragon_harvest_all)
            {
                var dragon_object = User.DragonContainer.GetDragonObject(m_SelectPetID);
                if (null == dragon_object)
                    return;

                if (false == dragon_object.HasDragonType(EnumDragonType.PRODUCT))
                    return;

                var product_reward_info = NDT.COMMON_DRAGON_PRODUCT_INFO.DRAGON_PRODUCT_TYPE_REWARD_INFO.Seek_EVOLUTION(dragon_object.evolution);
                if (null == product_reward_info)
                    return;

                int product_target_item_kind = (int)product_reward_info.CHARGE_REWARD.KIND;
                //for (int i = 0; i < m_product_slot_count.Count; ++i)
                //{
                //    if (false == OBJ.m_draogn_product_slots.Ex_IsValidIndex(i) || 0 >= m_product_slot_count[i])
                //        continue;

                //    GUI_ItemDirector.AddItemDirect_UI(OBJ.m_draogn_product_slots[i].m_root.transform, product_target_item_kind);
                //    EffectManager.Instance.PlayEffectOnce(OBJ.m_draogn_product_slots[i].m_root.gameObject, "FX_UI_TOUCH", Vector3.zero, Vector3.one, 0);
                //    AudioManager.Instance.PlaySFX("SFX/UI/sfx_ui_get_meat");
                //    OBJ.m_draogn_product_slots[i].m_root.Ex_SetActive(false);
                //}
            }
            else if (in_param is GameEvent.PROBABILITY_PARAM probability_param)
            {
                if (null == probability_param.probability_info)
                    return;

                m_promote_probability.Clear();
                foreach (var probability_value in probability_param.probability_info)
                {
                    if ((EnumProbabilityType)probability_value.PROBABILITYTYPE != EnumProbabilityType.DRAGON_PROMOTE)
                        continue;

                    m_promote_probability.Add(probability_value.PROBABILITYKIND, probability_value.PROBABILITYVALUE);
                }

                Refresh_CurrentDetailInfo();
            }
            else if (in_param is PET_LOCK lock_param)
            {
                if (m_SelectPetID != lock_param.m_petID)
                    return;

                var petObject = User.DragonPetContainer.GetDragonPetObject(m_SelectPetID);
                if (null == petObject)
                    return;

                ComposePetAlarm(petObject);
            }
            else if (in_param is DRAGON_MULTIPLE_HATCH_RETRY)
            {
                TryMultipleHatching();
            }
            //else if (in_param is GET_DRAGON_STATE_LIST param_ragon_list)
            //{
            //    m_dragon_state_list = new Dictionary<long, EnumDragonState>(param_ragon_list.m_dragon_state_list);
            //}
        }

        #region LocalData
        private void AddLocalSlot(long inpetID, int in_index, int in_count, ref int remain_count)
        {
            //var local_list = new List<int>();
            //remain_count = 0;

            //// 로컬데이터를 가져온다.
            //var local_array = PlayerPrefsX.GetIntArray(Util.Dragon.MakeLocalDataKey_Product(inpetID));
            //if (null != local_array)
            //    local_list.AddRange(local_array);

            //// 칸이 비어있다면 채워준다.
            //while (local_list.Count < OBJ.m_draogn_product_slots.Length)
            //    local_list.Add(0);

            //if (local_list.Ex_IsValidIndex(in_index))
            //{
            //    remain_count = Mathf.Max(0, local_list[in_index] + in_count);
            //    local_list[in_index] = remain_count;
            //}

            //PlayerPrefsX.SetData(Util.Dragon.MakeLocalDataKey_Product(inpetID), local_list.ToArray());
        }

        //private void RefreshLocalData_DragonProduct(long inpetID)
        //{
        //    // 실제 비늘 보유량을 가져온다.
        //    int current_count = (int)User.UserDragonProductManager.GetChargeAmount_DragonID(inpetID);

        //    var local_list = new List<int>();

        //    // 로컬데이터를 가져온다.
        //    var local_array = PlayerPrefsX.GetIntArray(Util.Dragon.MakeLocalDataKey_Product(inpetID));
        //    if (null != local_array)
        //        local_list.AddRange(local_array);

        //    // 비어있다면 채워준다.
        //    while (local_list.Count < OBJ.m_draogn_product_slots.Length)
        //        local_list.Add(0);

        //    int local_count = 0;
        //    for (int i = 0; i < local_list.Count; ++i)
        //        local_count += local_list[i];

        //    // 보유량이 같다면 갱신할 필요가 없다.
        //    if (local_count == current_count)
        //        return;

        //    Int32 change_count = current_count - local_count;
        //    //많다면 분배처리
        //    if (change_count > 0)
        //    {
        //        while (change_count > 0)
        //        {
        //            int min_idx = 0;
        //            int min_value = int.MaxValue;
        //            for (int i = 0; i < local_list.Count; ++i)
        //            {
        //                if (min_value > local_list[i])
        //                {
        //                    min_idx = i;
        //                    min_value = local_list[i];
        //                }
        //            }

        //            local_list[min_idx] += 1;
        //            change_count -= 1;
        //        }
        //    }
        //    else
        //    {
        //        while (change_count < 0)
        //        {
        //            int max_idx = 0;
        //            int max_value = -1;
        //            for (int i = 0; i < local_list.Count; ++i)
        //            {
        //                if (max_value < local_list[i])
        //                {
        //                    max_idx = i;
        //                    max_value = local_list[i];
        //                }
        //            }

        //            local_list[max_idx] -= 1;
        //            change_count += 1;
        //        }
        //    }

        //    PlayerPrefsX.SetData(Util.Dragon.MakeLocalDataKey_Product(inpetID), local_list.ToArray());
        //}

        #endregion


        void ClearProductEffect()
        {
            // 이펙트 제거
            foreach (var effect in m_product_effect_ids)
                EffectManager.Instance.DeleteEffect(effect);

            m_product_effect_ids.Clear();
        }


        public void PlayDragonFlyOutAnimation_Tutorial()
        {
            Play_PetAnimation("FlyUp");
        }


#region Hint

        public List<UIButtonEx> GetHintDragonLevelUPButton()
        {
            return new List<UIButtonEx>()
            {
                OBJ.m_button_feeding_non_training,
                OBJ.m_button_feeding
            };
        }

        public UIButtonEx GetHintDragonAwakeUPButton() { return OBJ.m_button_promote_move; }

        public UIButtonEx GetHintDragonAdventureButton() { return OBJ.m_button_go_dragon_adventure; }

#endregion Hint
    }
}