/**
 * @file AoIPDataStateTabletTypes.h
 *
 * @brief
 *
 */

#ifndef AOIPDATASTATETABLETTYPES_H_
#define AOIPDATASTATETABLETTYPES_H_

// LITTLE ENDIAN ONLY!

enum
{
    NAV_SELECT_CODE_VOR_L,
    NAV_SELECT_CODE_VOR_R,
    NAV_SELECT_CODE_ADF_L,
    NAV_SELECT_CODE_ADF_R,
};

enum
{
    APP_SELECT_CODE_APP_L,
    APP_SELECT_CODE_APP_R,
    APP_SELECT_CODE_MKR,
};

enum MicPos
{
    OFF_POS = 0,
    MIC_POS,
    FLT_POS
};


typedef struct AoIPDataStateTabletType
{
    uint8_t Reserved[4];
    uint8_t FunctionalStatus4;
    uint8_t FunctionalStatus3;
    uint8_t FunctionalStatus2;
    uint8_t FunctionalStatus1;

    uint8_t Pad0;

    uint8_t SAT_1_VolSel    :1;
    uint8_t SAT_2_VolSel    :1;
    uint8_t NAV_VolSel      :1;
    uint8_t APP_VolSel      :1;
    uint8_t SPK_VolSel      :1;
    uint8_t Pad1            :3;

    uint8_t VHF_L_VolSel    :1;
    uint8_t VHF_C_VolSel    :1;
    uint8_t VHF_R_VolSel    :1;
    uint8_t FLT_VolSel      :1;
    uint8_t CAB_VolSel      :1;
    uint8_t PA_VolSel       :1;
    uint8_t HF_L_VolSel     :1;
    uint8_t HF_R_VolSel     :1;

    uint8_t Nav_Select_Code :2;
    uint8_t App_Select_Code :2;
    uint8_t VBR_Select_Code :2;
    uint8_t Mic_Switch_Pos  :2;

    uint16_t VHF_C_Volume;
    uint16_t VHF_L_Volume;
    uint16_t FLT_Volume;
    uint16_t VHF_R_Volume;
    uint16_t PA_Volume;
    uint16_t CAB_Volume;
    uint16_t HF_R_Volume;
    uint16_t HF_L_Volume;
    uint16_t SAT_2_Volume;
    uint16_t SAT_1_Volume;
    uint16_t APP_Volume;
    uint16_t NAV_Volume;

    uint8_t SAT_1_Mic_On :1;
    uint8_t SAT_2_Mic_On :1;
    uint8_t Pad2 :6;

    uint8_t VHF_L_Mic_On :1;
    uint8_t VHF_C_Mic_On :1;
    uint8_t VHF_R_Mic_On :1;
    uint8_t FLT_Mic_On :1;
    uint8_t CAB_Mic_On :1;
    uint8_t PA_Mic_On :1;
    uint8_t HF_L_Mic_On :1;
    uint8_t HF_R_Mic_On :1;

    uint16_t SPK_Volume;



}__attribute__((packed)) AoIPDataStateTabletType_t;

#endif /* AOIPDATASTATETABLETTYPES_H_ */
