#include <msp430.h>

#define OLED_ADDR 0x3C

#define FLEX1_ESIK 250
#define FLEX2_ESIK 240

#define KAPI_KAPALI 1000
#define KAPI_ACIK   1800

#define ROLE_PIN BIT0
#define ROLE_AKTIF_LOW 1

#define DRY_VALUE 741
#define WET_VALUE 356

volatile unsigned int flex1 = 0;
volatile unsigned int flex2 = 0;
volatile unsigned int soil_raw = 0;
volatile unsigned int soil_yuzde = 0;

unsigned char pompa_acik = 0;
unsigned char oled_ok = 1;

void clock_1mhz_ayarla(void)
{
    WDTCTL = WDTPW | WDTHOLD;

    if (CALBC1_1MHZ != 0xFF)
    {
        DCOCTL = 0;
        BCSCTL1 = CALBC1_1MHZ;
        DCOCTL = CALDCO_1MHZ;
    }
}

void role_kapat(void)
{
#if ROLE_AKTIF_LOW
    P2OUT |= ROLE_PIN;
#else
    P2OUT &= ~ROLE_PIN;
#endif
}

void role_ac(void)
{
#if ROLE_AKTIF_LOW
    P2OUT &= ~ROLE_PIN;
#else
    P2OUT |= ROLE_PIN;
#endif
}

void role_baslat(void)
{
    P2SEL &= ~ROLE_PIN;
    P2SEL2 &= ~ROLE_PIN;
    P2DIR |= ROLE_PIN;
    role_kapat();
}

void servo_pwm_baslat(void)
{
    P2DIR |= BIT1;
    P2SEL |= BIT1;
    P2SEL2 &= ~BIT1;

    TA1CCR0 = 20000 - 1;
    TA1CCTL1 = OUTMOD_7;
    TA1CCR1 = KAPI_KAPALI;
    TA1CTL = TASSEL_2 | MC_1 | TACLR;
}

void i2c_init(void)
{
    P1SEL |= BIT6 | BIT7;
    P1SEL2 |= BIT6 | BIT7;

    UCB0CTL1 |= UCSWRST;

    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1 = UCSSEL_2 | UCSWRST;

    UCB0BR0 = 10;
    UCB0BR1 = 0;

    UCB0I2CSA = OLED_ADDR;

    UCB0CTL1 &= ~UCSWRST;
}

unsigned char i2c_wait_tx(void)
{
    unsigned int timeout = 60000;

    while (!(IFG2 & UCB0TXIFG))
    {
        if (UCB0STAT & UCNACKIFG)
        {
            UCB0STAT &= ~UCNACKIFG;
            return 0;
        }

        timeout--;

        if (timeout == 0)
        {
            return 0;
        }
    }

    return 1;
}

unsigned char i2c_wait_stop(void)
{
    unsigned int timeout = 60000;

    while (UCB0CTL1 & UCTXSTP)
    {
        timeout--;

        if (timeout == 0)
        {
            return 0;
        }
    }

    return 1;
}

unsigned char i2c_write(unsigned char control, unsigned char data)
{
    if (!oled_ok)
    {
        return 0;
    }

    if (!i2c_wait_stop())
    {
        oled_ok = 0;
        return 0;
    }

    UCB0CTL1 |= UCTR | UCTXSTT;

    if (!i2c_wait_tx())
    {
        UCB0CTL1 |= UCTXSTP;
        oled_ok = 0;
        return 0;
    }

    UCB0TXBUF = control;

    if (!i2c_wait_tx())
    {
        UCB0CTL1 |= UCTXSTP;
        oled_ok = 0;
        return 0;
    }

    UCB0TXBUF = data;

    if (!i2c_wait_tx())
    {
        UCB0CTL1 |= UCTXSTP;
        oled_ok = 0;
        return 0;
    }

    UCB0CTL1 |= UCTXSTP;

    if (!i2c_wait_stop())
    {
        oled_ok = 0;
        return 0;
    }

    return 1;
}

void oled_cmd(unsigned char cmd)
{
    i2c_write(0x00, cmd);
}

void oled_data(unsigned char data)
{
    i2c_write(0x40, data);
}

void oled_set_pos(unsigned char x, unsigned char page)
{
    if (!oled_ok)
    {
        return;
    }

    oled_cmd(0xB0 + page);
    oled_cmd(0x00 + (x & 0x0F));
    oled_cmd(0x10 + ((x >> 4) & 0x0F));
}

void oled_clear_area(unsigned char x, unsigned char page, unsigned char width, unsigned char page_count)
{
    unsigned char p;
    unsigned char i;

    if (!oled_ok)
    {
        return;
    }

    for (p = 0; p < page_count; p++)
    {
        oled_set_pos(x, page + p);

        for (i = 0; i < width; i++)
        {
            oled_data(0x00);
        }
    }
}

void oled_clear(void)
{
    unsigned char page;
    unsigned char col;

    if (!oled_ok)
    {
        return;
    }

    for (page = 0; page < 8; page++)
    {
        oled_set_pos(0, page);

        for (col = 0; col < 128; col++)
        {
            oled_data(0x00);
        }
    }
}

void oled_init(void)
{
    oled_ok = 1;

    __delay_cycles(200000);

    oled_cmd(0xAE);
    oled_cmd(0xD5);
    oled_cmd(0x80);
    oled_cmd(0xA8);
    oled_cmd(0x3F);
    oled_cmd(0xD3);
    oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D);
    oled_cmd(0x14);
    oled_cmd(0x20);
    oled_cmd(0x02);
    oled_cmd(0xA1);
    oled_cmd(0xC8);
    oled_cmd(0xDA);
    oled_cmd(0x12);
    oled_cmd(0x81);
    oled_cmd(0xCF);
    oled_cmd(0xD9);
    oled_cmd(0xF1);
    oled_cmd(0xDB);
    oled_cmd(0x40);
    oled_cmd(0xA4);
    oled_cmd(0xA6);
    oled_cmd(0x2E);
    oled_cmd(0xAF);

    oled_clear();
}

unsigned int adc_tek_oku(unsigned int kanal, unsigned int bit)
{
    ADC10CTL0 &= ~ENC;

    ADC10AE0 |= bit;
    ADC10CTL1 = kanal | ADC10DIV_3;
    ADC10CTL0 = SREF_0 | ADC10SHT_3 | ADC10ON;

    __delay_cycles(1000);

    ADC10CTL0 |= ENC | ADC10SC;

    while (ADC10CTL1 & ADC10BUSY);

    return ADC10MEM;
}

unsigned int adc_ortalama_oku(unsigned int kanal, unsigned int bit)
{
    unsigned long toplam = 0;
    unsigned int i;

    adc_tek_oku(kanal, bit);

    for (i = 0; i < 8; i++)
    {
        toplam += adc_tek_oku(kanal, bit);
        __delay_cycles(1000);
    }

    return toplam / 8;
}

unsigned int soil_percent(unsigned int raw)
{
    long percent;

    percent = ((long)DRY_VALUE - raw) * 100;
    percent = percent / ((long)DRY_VALUE - WET_VALUE);

    if (percent < 0)
    {
        percent = 0;
    }

    if (percent > 100)
    {
        percent = 100;
    }

    return (unsigned int)percent;
}

const unsigned char digit_font[11][5] = {
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x63, 0x13, 0x08, 0x64, 0x63}
};

void oled_draw_digit(unsigned char x, unsigned char page, unsigned char digit)
{
    unsigned char i;

    if (!oled_ok)
    {
        return;
    }

    oled_set_pos(x, page);

    for (i = 0; i < 5; i++)
    {
        oled_data(digit_font[digit][i]);
    }

    oled_data(0x00);
}

void oled_draw_percent(unsigned int percent)
{
    unsigned char x = 42;

    if (!oled_ok)
    {
        return;
    }

    oled_clear_area(36, 6, 60, 1);

    if (percent >= 100)
    {
        oled_draw_digit(x, 6, 1);
        oled_draw_digit(x + 8, 6, 0);
        oled_draw_digit(x + 16, 6, 0);
        oled_draw_digit(x + 26, 6, 10);
    }
    else if (percent >= 10)
    {
        oled_draw_digit(x + 8, 6, percent / 10);
        oled_draw_digit(x + 16, 6, percent % 10);
        oled_draw_digit(x + 26, 6, 10);
    }
    else
    {
        oled_draw_digit(x + 16, 6, percent);
        oled_draw_digit(x + 26, 6, 10);
    }
}

void oled_draw_raw(unsigned int raw)
{
    unsigned char x = 38;

    if (!oled_ok)
    {
        return;
    }

    oled_clear_area(34, 7, 70, 1);

    oled_draw_digit(x, 7, (raw / 1000) % 10);
    oled_draw_digit(x + 8, 7, (raw / 100) % 10);
    oled_draw_digit(x + 16, 7, (raw / 10) % 10);
    oled_draw_digit(x + 24, 7, raw % 10);
}

const unsigned char dry_face[4][32] = {
    {
        0x00,0x00,0xE0,0xF8,0xFC,0x1E,0x0E,0x07,
        0x07,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
        0x03,0x03,0x03,0x03,0x03,0x03,0x07,0x07,
        0x0E,0x1E,0xFC,0xF8,0xE0,0x00,0x00,0x00
    },
    {
        0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,
        0x18,0x3C,0x3C,0x18,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x18,0x3C,0x3C,0x18,
        0x00,0x00,0x00,0x03,0xFF,0xFE,0x00,0x00
    },
    {
        0x00,0x7F,0xFF,0xC0,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x80,0xC0,0x60,0x20,
        0x20,0x60,0xC0,0x80,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0xC0,0xFF,0x7F,0x00,0x00
    },
    {
        0x00,0x00,0x07,0x1F,0x3F,0x78,0x70,0xE0,
        0xE0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
        0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xE0,0xE0,
        0x70,0x78,0x3F,0x1F,0x07,0x00,0x00,0x00
    }
};

const unsigned char normal_face[4][32] = {
    {
        0x00,0x00,0xE0,0xF8,0xFC,0x1E,0x0E,0x07,
        0x07,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
        0x03,0x03,0x03,0x03,0x03,0x03,0x07,0x07,
        0x0E,0x1E,0xFC,0xF8,0xE0,0x00,0x00,0x00
    },
    {
        0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,
        0x30,0x78,0x78,0x30,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,
        0x00,0x00,0x00,0x03,0xFF,0xFE,0x00,0x00
    },
    {
        0x00,0x7F,0xFF,0xC0,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xE0,
        0xE0,0xE0,0xC0,0x80,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0xC0,0xFF,0x7F,0x00,0x00
    },
    {
        0x00,0x00,0x07,0x1F,0x3F,0x78,0x70,0xE0,
        0xE0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
        0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xE0,0xE0,
        0x70,0x78,0x3F,0x1F,0x07,0x00,0x00,0x00
    }
};

const unsigned char wet_face[4][32] = {
    {
        0x00,0x00,0xE0,0xF8,0xFC,0x1E,0x0E,0x07,
        0x07,0x03,0x03,0x03,0x03,0x03,0x03,0x03,
        0x03,0x03,0x03,0x03,0x03,0x03,0x07,0x07,
        0x0E,0x1E,0xFC,0xF8,0xE0,0x00,0x00,0x00
    },
    {
        0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,
        0x30,0x78,0x78,0x30,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,
        0x00,0x00,0x00,0x03,0xFF,0xFE,0x00,0x00
    },
    {
        0x00,0x7F,0xFF,0xC0,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x80,0xC0,0xE0,0xE0,0xE0,
        0xE0,0xE0,0xE0,0xC0,0x80,0x00,0x00,0x00,
        0x00,0x00,0x00,0xC0,0xFF,0x7F,0x00,0x00
    },
    {
        0x00,0x00,0x07,0x1F,0x3F,0x78,0x70,0xE0,
        0xE0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,
        0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xE0,0xE0,
        0x70,0x78,0x3F,0x1F,0x07,0x00,0x00,0x00
    }
};

void draw_face(const unsigned char face[4][32], unsigned char x, unsigned char page)
{
    unsigned char p;
    unsigned char c;

    if (!oled_ok)
    {
        return;
    }

    for (p = 0; p < 4; p++)
    {
        oled_set_pos(x, page + p);

        for (c = 0; c < 32; c++)
        {
            oled_data(face[p][c]);
        }
    }
}

void draw_bar(unsigned int percent)
{
    unsigned char i;
    unsigned char fill;

    if (!oled_ok)
    {
        return;
    }

    fill = (unsigned char)percent;

    oled_set_pos(14, 5);

    oled_data(0xFF);

    for (i = 0; i < 100; i++)
    {
        if (i < fill)
        {
            oled_data(0x7E);
        }
        else
        {
            oled_data(0x42);
        }
    }

    oled_data(0xFF);
}

void screen_update(unsigned int percent, unsigned int raw)
{
    if (!oled_ok)
    {
        return;
    }

    oled_clear_area(48, 0, 32, 4);

    if (percent < 30)
    {
        draw_face(dry_face, 48, 0);
    }
    else if (percent < 70)
    {
        draw_face(normal_face, 48, 0);
    }
    else
    {
        draw_face(wet_face, 48, 0);
    }

    draw_bar(percent);
    oled_draw_percent(percent);
    oled_draw_raw(raw);
}

int main(void)
{
    unsigned char flex1_bukuk;
    unsigned char flex2_bukuk;

    unsigned int soil_sayac = 0;
    unsigned int ekran_sayac = 0;
    unsigned int son_raw = 9999;
    unsigned int son_yuzde = 9999;

    clock_1mhz_ayarla();

    P1DIR &= ~(BIT0 | BIT1 | BIT5);
    P1SEL &= ~(BIT0 | BIT1 | BIT5);
    P1SEL2 &= ~(BIT0 | BIT1 | BIT5);
    ADC10AE0 |= BIT0 | BIT1 | BIT5;

    role_baslat();
    servo_pwm_baslat();

    TA1CCR1 = KAPI_KAPALI;
    pompa_acik = 0;
    role_kapat();

    i2c_init();
    oled_init();

    soil_raw = adc_ortalama_oku(INCH_5, BIT5);
    soil_yuzde = soil_percent(soil_raw);
    screen_update(soil_yuzde, soil_raw);
    son_raw = soil_raw;
    son_yuzde = soil_yuzde;

    while (1)
    {
        flex1 = adc_ortalama_oku(INCH_0, BIT0);
        flex2 = adc_ortalama_oku(INCH_1, BIT1);

        flex1_bukuk = flex1 > FLEX1_ESIK;
        flex2_bukuk = flex2 > FLEX2_ESIK;

        if (!flex1_bukuk && !flex2_bukuk)
        {
            TA1CCR1 = KAPI_ACIK;
        }
        else if (flex1_bukuk && flex2_bukuk)
        {
            TA1CCR1 = KAPI_KAPALI;
            pompa_acik = 0;
            
        }
        else if (!flex1_bukuk && flex2_bukuk)
        {
            pompa_acik = 1;
        }
        else if (flex1_bukuk && !flex2_bukuk)
        {
            pompa_acik = 0;
        }

        if (pompa_acik == 1)
        {
            role_ac();
        }
        else
        {
            role_kapat();
        }

        soil_sayac++;
        ekran_sayac++;

        if (soil_sayac >= 3)
        {
            soil_raw = adc_ortalama_oku(INCH_5, BIT5);
            soil_yuzde = soil_percent(soil_raw);
            soil_sayac = 0;
        }

        if (ekran_sayac >= 10)
        {
            if ((soil_raw != son_raw) || (soil_yuzde != son_yuzde))
            {
                screen_update(soil_yuzde, soil_raw);
                son_raw = soil_raw;
                son_yuzde = soil_yuzde;
            }

            ekran_sayac = 0;
        }

        __delay_cycles(20000);
    }
}