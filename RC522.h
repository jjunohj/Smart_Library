#ifndef __RC522_CONFIG_H
#define	__RC522_CONFIG_H

/////////////////////////////////////////////////////////////////////
//MF522 Command word
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //현재 명령 취소
#define PCD_AUTHENT           0x0E               //인증 키
#define PCD_RECEIVE           0x08               //데이터 수신
#define PCD_TRANSMIT          0x04               //데이터 전송
#define PCD_TRANSCEIVE        0x0C               //데이터 전송 및 수신
#define PCD_RESETPHASE        0x0F               //재설정
#define PCD_CALCCRC           0x03               //CRC 계산

/////////////////////////////////////////////////////////////////////
//Mifare_One 카드 Command word
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //수신기간 내에 대기중인 카드 대기열 찾기
#define PICC_REQALL           0x52               // 수신기 안에 있는 모든 카드 찾기
#define PICC_ANTICOLL1        0x93               //충돌 방지
#define PICC_ANTICOLL2        0x95

#define PICC_AUTHENT1A        0x60               //A키 검증
#define PICC_AUTHENT1B        0x61               //B키 검증
#define PICC_READ             0x30               //블록 읽기
#define PICC_WRITE            0xA0               //블록 쓰기
#define PICC_DECREMENT        0xC0               //차감
#define PICC_INCREMENT        0xC1               //충전
#define PICC_RESTORE          0xC2               //블록 데이터를 버퍼로 복원
#define PICC_TRANSFER         0xB0               //버퍼에 있는 데이터 보관
#define PICC_HALT             0x50               //카드 비활성화

/////////////////////////////////////////////////////////////////////
//MF522 FIFO 길이 정의
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO 크기 = 64바이트
#define MAXRLEN  18

/////////////////////////////////////////////////////////////////////
//MF522 레지스터 정의
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00
#define     CommandReg            0x01
#define     ComIEnReg             0x02
#define     DivlEnReg             0x03
#define     ComIrqReg             0x04
#define     DivIrqReg             0x05
#define     ErrorReg              0x06
#define     Status1Reg            0x07
#define     Status2Reg            0x08
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2
#define     RFU20                 0x20
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39
#define     TestDAC2Reg           0x3A
#define     TestADCReg            0x3B
#define     RFU3C                 0x3C
#define     RFU3D                 0x3D
#define     RFU3E                 0x3E
#define     RFU3F		  		        0x3F

/////////////////////////////////////////////////////////////////////
//MF522 통신 에러 코드
/////////////////////////////////////////////////////////////////////
#define 	MI_OK                 0x26
#define 	MI_NOTAGERR           0xcc
#define 	MI_ERR                0xbb

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void RC522_setup(uint8_t rst);

void PcdReset ( void );
void M500PcdConfigISOType ( uint8_t type );
char PcdRequest ( uint8_t req_code, uint8_t* pTagType );
char PcdAnticoll ( uint8_t* pSnr);
char PcdHalt ( void );
char PcdSelect ( uint8_t* pSnr );
void CalulateCRC ( uint8_t* pIndata, uint8_t len, uint8_t* pOutData );
char PcdAuthState ( uint8_t auth_mode, uint8_t addr, uint8_t* pKey, uint8_t* pSnr );
char PcdRead ( uint8_t addr, uint8_t* pData );
char PcdWrite ( uint8_t addr, uint8_t* pData );
char PcdValue ( uint8_t dd_mode, uint8_t addr, uint8_t* pValue );
char PcdBakValue ( uint8_t sourceaddr, uint8_t goaladdr );
char PcdComMF522 ( uint8_t Command, uint8_t* pInData, uint8_t InLenByte, uint8_t* pOutData, uint32_t* pOutLenBit );




#endif