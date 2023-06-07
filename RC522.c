#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <string.h>
#include "RC522.h"

uint8_t rst_pin;
void RC522_setup(uint8_t rst) {
    rst_pin = rst;
    wiringPiSetup();
    pinMode(rst_pin, OUTPUT);
    digitalWrite(rst_pin, 1);
    wiringPiSPISetup(0, 100000);
}

void RC522_RST(uint8_t v) {
    digitalWrite(rst_pin, v);
}

/**
 * @brief Reads an RC522 register
 * @param ucAddress, register address
 * @retval Current value of the register
 */
uint8_t ReadRawRC(uint8_t ucAddress) {
    static uint8_t _buf1[0x2];
    uint8_t ucAddr, ucReturn;

    ucAddr = ((ucAddress << 1) & 0x7E) | 0x80;
    _buf1[0] = ucAddr;
    wiringPiSPIDataRW(0, _buf1, 2);
    ucReturn = _buf1[1];

    return ucReturn;
}

/**
 * @brief Writes to an RC522 register
 * @param ucAddress, register address
 * @param ucValue, value to write to the register
 * @retval None
 */
void WriteRawRC(uint8_t ucAddress, uint8_t ucValue) {
    static uint8_t _buf1[0x2];
    uint8_t ucAddr;

    ucAddr = (ucAddress << 1) & 0x7E;
    _buf1[0] = ucAddr;
    _buf1[1] = ucValue;
    wiringPiSPIDataRW(0, _buf1, 2);
}

/**
 * @brief Sets a bit in an RC522 register
 * @param ucReg, register address
 * @param ucMask, bit value to set
 * @retval None
 */
void SetBitMask(uint8_t ucReg, uint8_t ucMask) {
    uint8_t ucTemp;

    ucTemp = ReadRawRC(ucReg);
    WriteRawRC(ucReg, ucTemp | ucMask); // set bit mask
}

/**
 * @brief Clears a bit in an RC522 register
 * @param ucReg, register address
 * @param ucMask, bit value to clear
 * @retval None
 */
void ClearBitMask(uint8_t ucReg, uint8_t ucMask) {
    uint8_t ucTemp;

    ucTemp = ReadRawRC(ucReg);
    WriteRawRC(ucReg, ucTemp & (~ucMask)); // clear bit mask
}

/**
 * @brief Turns on the antenna of the RC522
 * @param None
 * @retval None
 */
void PcdAntennaOn(void) {
    uint8_t uc;

    uc = ReadRawRC(TxControlReg);
    if (!(uc & 0x03)) {
        SetBitMask(TxControlReg, 0x03);
    }
}

/**
 * @brief Turns off the antenna of the RC522
 * @param None
 * @retval None
 */
void PcdAntennaOff(void) {
    ClearBitMask(TxControlReg, 0x03);
}

/**
 * @brief Resets the RC522
 * @param None
 * @retval None
 */
void PcdReset(void) {
    RC522_RST(1);
    delayMicroseconds(1);
    RC522_RST(0);
    delayMicroseconds(1);
    RC522_RST(1);
    delayMicroseconds(1);
    WriteRawRC(CommandReg, 0x0F);
    while (ReadRawRC(CommandReg) & 0x10);
    delayMicroseconds(1);
    //Configures commonly used modes for sending and receiving and communication with Mifare cards, initializes CRC value to 0x6363
    WriteRawRC(ModeReg, 0x3D);
    WriteRawRC(TReloadRegL, 30);      // Low 16 bits of the 16- bit timer
    WriteRawRC(TReloadRegH, 0);	      // High 16 bits of the 16-bit timer
    WriteRawRC(TModeReg, 0x8D);	      // Defines settings for internal timer
    WriteRawRC(TPrescalerReg, 0x3E);   // Sets the timer's prescaler coefficient
    WriteRawRC(TxAutoReg, 0x40);	      // Modulates the transmitted signal 100% ASK
}

/**
 * @brief Sets the operating mode of the RC522
 * @param ucType, operating mode
 * @retval None
 */
void M500PcdConfigISOType(uint8_t ucType) {
    if (ucType == 'A') {                   //ISO14443_A
        ClearBitMask(Status2Reg, 0x08);
        WriteRawRC(ModeReg, 0x3D);         //3F
        WriteRawRC(RxSelReg, 0x86);        //84
        WriteRawRC(RFCfgReg, 0x7F);        //4F
        WriteRawRC(TReloadRegL, 30);
        WriteRawRC(TReloadRegH, 0);
        WriteRawRC(TModeReg, 0x8D);
        WriteRawRC(TPrescalerReg, 0x3E);
        delayMicroseconds(2);
        PcdAntennaOn(); //Open antenna
    }
}

/**
 * @brief Communicates with an ISO14443 card using RC522
 * @param ucCommand, RC522 command word
 * @param pInData, data to send to the card through RC522
 * @param ucInLenByte, byte length of the data to send
 * @param pOutData, returned data from the card
 * @param pOutLenBit, bit length of the returned data
 * @retval Status value = MI_OK, success
 */
char PcdComMF522(uint8_t ucCommand, uint8_t* pInData, uint8_t ucInLenByte, uint8_t* pOutData, uint32_t* pOutLenBit) 
{
    char cStatus = MI_ERR;
    uint8_t ucIrqEn   = 0x00;
    uint8_t ucWaitFor = 0x00;
    uint8_t ucLastBits;
    uint8_t ucN;
    uint32_t ul;

    switch (ucCommand) {
        case PCD_AUTHENT:
            ucIrqEn   = 0x12;
            ucWaitFor = 0x10;
            break;

        case PCD_TRANSCEIVE:
            ucIrqEn   = 0x77;
            ucWaitFor = 0x30;
            break;

        default:
            break;
    }

    WriteRawRC(ComIEnReg, ucIrqEn | 0x80);
    ClearBitMask(ComIrqReg, 0x80);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (ul = 0; ul < ucInLenByte; ul++) {
        WriteRawRC(FIFODataReg, pInData[ul]);
    }

    WriteRawRC(CommandReg, ucCommand);

    if (ucCommand == PCD_TRANSCEIVE) {
        SetBitMask(BitFramingReg, 0x80);
    }

    ul = 1000;

    do {
        ucN = ReadRawRC(ComIrqReg);
        ul--;
    } while ((ul != 0) && (!(ucN & 0x01)) && (!(ucN & ucWaitFor)));

    ClearBitMask(BitFramingReg, 0x80);

    if (ul != 0) {
        if (!(ReadRawRC(ErrorReg) & 0x1B)) {
            cStatus = MI_OK;

            if (ucN & ucIrqEn & 0x01) {
                cStatus = MI_NOTAGERR;
            }

            if (ucCommand == PCD_TRANSCEIVE) {
                ucN = ReadRawRC(FIFOLevelReg);
                ucLastBits = ReadRawRC(ControlReg) & 0x07;

                if (ucLastBits) {
                    *pOutLenBit = (ucN - 1) * 8 + ucLastBits;
                } else {
                    *pOutLenBit = ucN * 8;
                }

                if (ucN == 0) {
                    ucN = 1;
                }

                if (ucN > MAXRLEN) {
                    ucN = MAXRLEN;
                }

                for (ul = 0; ul < ucN; ul++) {
                    pOutData[ul] = ReadRawRC(FIFODataReg);
                }
            }
		} else {
            cStatus = MI_ERR;
        }
    }

    SetBitMask(ControlReg, 0x80);
    WriteRawRC(CommandReg, PCD_IDLE);

    return cStatus;
}

char PcdRequest(uint8_t ucReq_code, uint8_t* pTagType) { //Request cards, read card type number
    char cStatus;
    uint8_t ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x07);
    SetBitMask(TxControlReg, 0x03);

    ucComMF522Buf[0] = ucReq_code;
    cStatus = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &ulLen);

    if ((cStatus == MI_OK) && (ulLen == 0x10)) {
        * pTagType     = ucComMF522Buf[0];
        *(pTagType+1) = ucComMF522Buf[1];
    } else {
        cStatus = MI_ERR;
    }

    return cStatus;
}

char PcdAnticoll(uint8_t* pSnr ){ //Anti-collision detection, reading selected card serial number card
    char cStatus;
    uint8_t uc, ucSnr_check = 0;
    uint8_t ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ClearBitMask(Status2Reg, 0x08);
    WriteRawRC(BitFramingReg, 0x00);
    ClearBitMask(CollReg, 0x80);

    ucComMF522Buf[0] = 0x93;
    ucComMF522Buf[1] = 0x20;
    cStatus = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &ulLen);

    if (cStatus == MI_OK) {
        for (uc = 0; uc < 4; uc++) {
            *(pSnr + uc)  = ucComMF522Buf[uc];
            ucSnr_check ^= ucComMF522Buf[uc];
        }

        if (ucSnr_check != ucComMF522Buf[uc]) {
            cStatus = MI_ERR;
        }
    }

    SetBitMask(CollReg, 0x80);

    return cStatus;
}

void CalulateCRC(uint8_t* pIndata, uint8_t ucLen, uint8_t* pOutData) {
    uint8_t uc, ucN;
    ClearBitMask(DivIrqReg, 0x04);
    WriteRawRC(CommandReg, PCD_IDLE);
    SetBitMask(FIFOLevelReg, 0x80);

    for (uc = 0; uc < ucLen; uc++) {
        WriteRawRC(FIFODataReg, *(pIndata+uc));
    }

    WriteRawRC(CommandReg, PCD_CALCCRC);
    uc = 0xFF;

    do {
        ucN = ReadRawRC(DivIrqReg);
        uc--;
    } while ((uc != 0) && !(ucN & 0x04));

    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

char PcdSelect(uint8_t* pSnr) {
    char ucN;
    uint8_t uc, ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;

    for (uc = 0; uc < 4; uc++) {
        ucComMF522Buf[uc+2] = *(pSnr+uc);
        ucComMF522Buf[6]   ^= *(pSnr+uc);
    }

    CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);
    ClearBitMask(Status2Reg, 0x08);

    ucN = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &ulLen);

    if ((ucN == MI_OK) && (ulLen == 0x18)) {
        ucN = MI_OK;
    } else {
        ucN = MI_ERR;
    }

    return ucN;
}

char PcdAuthState(uint8_t ucAuth_mode, uint8_t ucAddr, uint8_t* pKey, uint8_t* pSnr) {
    char cStatus;
    uint8_t uc, ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ucComMF522Buf[0] = ucAuth_mode;
    ucComMF522Buf[1] = ucAddr;

    for (uc = 0; uc < 6; uc++) {
        ucComMF522Buf[uc+2] = *(pKey+uc);
    }

    for (uc = 0; uc < 6; uc++) {
        ucComMF522Buf[uc+8] = *(pSnr+uc);
    }

    cStatus = PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &ulLen);

    if ((cStatus != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08))) {
        cStatus = MI_ERR;
    }

    return cStatus;
}

char PcdWrite(uint8_t ucAddr, uint8_t* pData) { // Write block data
    char cStatus;
    uint8_t uc, ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = ucAddr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    cStatus = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &ulLen);

    if ((cStatus == MI_OK) && (ulLen == 4) && ((ucComMF522Buf[0] & 0x0F) == 0x0A)) {
        for (uc = 0; uc < 16; uc++) {
            ucComMF522Buf[uc] = *(pData+uc);
        }

        CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);
        cStatus = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &ulLen);

        if ((cStatus != MI_OK) || (ulLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) {
            cStatus = MI_ERR;
        }
    } else {
        cStatus = MI_ERR;
    }

    return cStatus;
}

char PcdRead(uint8_t ucAddr, uint8_t* pData) { // Read block data
    char cStatus;
    uint8_t uc, ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = ucAddr;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    cStatus = PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &ulLen);

    if ((cStatus == MI_OK) && (ulLen == 0x90)) {
        for (uc = 0; uc < 16; uc++) {
            *(pData + uc) = ucComMF522Buf[uc];
        }
    } else {
        cStatus = MI_ERR;
    }

    return cStatus;
}

char PcdHalt() { // Command the cards into sleep mode
    uint8_t ucComMF522Buf[MAXRLEN];
    uint32_t ulLen;

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &ulLen);

    return MI_OK;
}

void IC_CMT(uint8_t* UID, uint8_t* KEY, uint8_t RW, uint8_t* Dat) { // IC Card Communication
    uint8_t ucArray_ID[4] = {0};
    PcdRequest(0x52, ucArray_ID);
    PcdAnticoll(ucArray_ID);
    PcdSelect(UID);
    PcdAuthState(0x60, 0x10, KEY, UID);

    if (RW) {
        PcdRead(0x10, Dat);
    } else {
        PcdWrite(0x10, Dat);
    }

    PcdHalt();
}
