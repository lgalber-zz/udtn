/*
 * Copyright (c) 2013, TU Braunschweig
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Contiki system setup
 * \author
 *      Robert Hartung
 *      Enrico Joerns <e.joerns@tu-bs.de>
 */

#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)

/* If defined 1, prints boot screen informations.
 * @note: Adds about 600 bytes to program size
 */
#ifndef ANNOUNCE_BOOT
#define ANNOUNCE_BOOT 1
#endif

#if ANNOUNCE_BOOT
#define PRINTA(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTA(...)
#endif

/* If defined 1, prints debug infos. */
#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define PRINTD(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTD(...)
#endif

/* Track interrupt flow through mac, rdc and radio driver */
#if DEBUGFLOWSIZE
uint8_t debugflowsize, debugflow[DEBUGFLOWSIZE];
#define DEBUGFLOW(c) if (debugflowsize<(DEBUGFLOWSIZE-1)) debugflow[debugflowsize++]=c
#else
#define DEBUGFLOW(c)
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "dev/watchdog.h"

// settings manager
#include "lib/settings.h"

// sensors
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/acc-sensor.h"
#include "dev/gyro-sensor.h"
#include "dev/pressure-sensor.h"
#include "dev/battery-sensor.h"
#include "dev/at45db.h"

#include "uip.h"

#if RF230BB        //radio driver using contiki core mac
#include "radio/rf230bb/rf230bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/sicslowpan.h"

#else              //radio driver using Atmel/Cisco 802.15.4'ish MAC
#include "mac.h"
#include "sicslowmac.h"
#include "sicslowpan.h"
#include "ieee-15-4-manager.h"
#endif /*RF230BB*/

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "sys/node-id.h"

#include "dev/rs232.h"
#include "dev/serial-line.h"
#include "dev/slip.h"

#if AVR_WEBSERVER
#include "httpd-fs.h"
#include "httpd-cgi.h"
#endif

#ifdef COFFEE_FILES
#include "cfs/cfs-coffee.h"
#endif

#if WITH_UIP6
#include "net/uip-ds6.h"
#endif /* WITH_UIP6 */

#include "net/rime.h"

// Apps 
#if defined(APP_SETTINGS_DELETE)
#include "settings_delete.h"
#elif defined(APP_SETTINGS_SET)
#include "settings_set.h"
#endif

/* Get periodic prints from idle loop, from clock seconds or rtimer interrupts */
/* Use of rtimer will conflict with other rtimer interrupts such as contikimac radio cycling */
/* STAMPS will print ENERGEST outputs if that is enabled. */
#ifndef PERIODIC_ENABLE
#define PERIODICPRINTS 1
#else
#define PERIODICPRINTS PERIODIC_ENABLE
#endif

/** Enables pings with given interval [seconds] */
#ifndef PERIODIC_CONF_PINGS
#define PER_PINGS 0
#else
#define PER_PINGS PERIODIC_CONF_PINGS
#if PER_PINGS > 0 && !UIP_CONF_IPV6
#error Periodic ping only supported for IPv6
#endif
#endif
/** Enables route prints with given interval [seconds] */
#ifndef PERIODIC_CONF_ROUTES
#define PER_ROUTES 0
#else
#define PER_ROUTES PERIODIC_CONF_ROUTES
#if PER_ROUTES > 0 && !UIP_CONF_IPV6
#error Periodic routes only supported for IPv6
#endif
#endif
/** Enables time stamps with given interval [seconds] */
#ifndef PERIODIC_CONF_STAMPS
#define PER_STAMPS 60
#else
#define PER_STAMPS PERIODIC_CONF_STAMPS
#endif
/** Activates stack monitor with given interval [seconds] */
#ifndef STACKMONITOR
#define STACKMONITOR 60
#endif


#ifndef USART_BAUD_INGA
#define USART_BAUD_INGA USART_BAUD_19200
#endif

/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if (__AVR_LIBC_VERSION__ >= 10700UL)
/* The proper way to set the signature is */
#include <avr/signature.h>
#else

/* signature API not available before avr-lib-1.7.0. Do it manually.*/
typedef struct {
  const unsigned char B2;
  const unsigned char B1;
  const unsigned char B0;
} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE = {
  .B2 = 0x05, //SIGNATURE_2, //ATMEGA1284p
  .B1 = 0x97, //SIGNATURE_1, //128KB flash
  .B0 = 0x1E, //SIGNATURE_0, //Atmel
};
#endif

/** Fuse-settings:
 * JTAG, SPI enabled, Internal RC osc, Boot flash size 4K,
 * 6CK+65msec delay, brownout disabled
 */
FUSES = {
  .low = 0xe2,
  .high = 0x99, // default
  .extended = 0xff, // default
};

#if CONTIKI_CONF_RANDOM_MAC
/** Get a pseudo random number using the ADC */
static uint8_t
rng_get_uint8(void)
{
  uint8_t i, j;
  ADCSRA = 1 << ADEN; //Enable ADC, not free running, interrupt disabled, fastest clock
  for (i = 0; i < 4; i++) {
    ADMUX = 0; //toggle reference to increase noise
    ADMUX = 0x1E; //Select AREF as reference, measure 1.1 volt bandgap reference.
    ADCSRA |= 1 << ADSC; //Start conversion
    while (ADCSRA & (1 << ADSC)); //Wait till done
    j = (j << 2) + ADC;
  }
  ADCSRA = 0; //Disable ADC
  PRINTD("rng issues %d\n", j);
  return j;
}
/*----------------------------------------------------------------------------*/
static void
generate_new_eui64(uint8_t eui64[8])
{
  eui64[0] = 0x02;
  eui64[1] = rng_get_uint8();
  eui64[2] = rng_get_uint8();
  eui64[3] = 0xFF;
  eui64[4] = 0xFE;
  eui64[5] = rng_get_uint8();
  eui64[6] = rng_get_uint8();
  eui64[7] = rng_get_uint8();
}
#endif /* CONTIKI_CONF_RANDOM_MAC */
/*----------------------------------------------------------------------------*/
// implements log function from uipopt.h
void
uip_log(char *msg)
{
  printf("%s\n", msg);
}
/*----------------------------------------------------------------------------*/
// config variables, preset with default values
uint8_t radio_tx_power = RADIO_TX_POWER;
uint8_t radio_channel = RADIO_CHANNEL;
uint16_t pan_id = RADIO_PAN_ID;
uint8_t eui64_addr[8] = {NODE_EUI64};
/*----------------------------------------------------------------------------*/
// implement sys/node-id.h interface
unsigned short node_id = NODE_ID;
/*----------------------------------------------------------------------------*/
void node_id_restore(void) {
  node_id = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
}
/*----------------------------------------------------------------------------*/
void node_id_burn(unsigned short node_id) {
  if (settings_set_uint16(SETTINGS_KEY_PAN_ADDR, (uint16_t) node_id) == SETTINGS_STATUS_OK) {
    uint16_t settings_nodeid = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
    PRINTF("New Node ID: %04X\n", settings_nodeid);
  } else {
    PRINTF("Error: Error while writing NodeID to EEPROM\n");
  }
}
/*----------------------------------------------------------------------------*/
void
platform_radio_init(void)
{

  /*******************************************************************************
   * Load settings from EEPROM if not set manually
   ******************************************************************************/

  // PAN_ID
#ifndef RADIO_CONF_PAN_ID
  if (settings_check(SETTINGS_KEY_PAN_ID, 0) == true) {
    pan_id = settings_get_uint16(SETTINGS_KEY_PAN_ID, 0);
  } else {
    PRINTD("PanID not in EEPROM - using default\n");
  }
#endif

  // PAN_ADDR/NODE_ID
#ifndef NODE_CONF_ID
  if (settings_check(SETTINGS_KEY_PAN_ADDR, 0) == true) {
    node_id = settings_get_uint16(SETTINGS_KEY_PAN_ADDR, 0);
  } else {
    PRINTD("NodeID/PanAddr not in EEPROM - using default\n");
  }
#endif

  // TX_POWER
#ifndef RADIO_CONF_TX_POWER
  if (settings_check(SETTINGS_KEY_TXPOWER, 0) == true) {
    radio_tx_power = settings_get_uint8(SETTINGS_KEY_TXPOWER, 0);
  } else {
    PRINTD("Radio TXPower not in EEPROM - using default\n");
  }
#endif

  // CHANNEL
#ifndef RADIO_CONF_CHANNEL
  if (settings_check(SETTINGS_KEY_CHANNEL, 0) == true) {
    radio_channel = settings_get_uint8(SETTINGS_KEY_CHANNEL, 0);
  } else {
    PRINTD("Radio Channel not in EEPROM - using default\n");
  }
#endif

  // EUI64 ADDR
#ifndef NODE_CONF_EUI64
  // if setting not set or invalid data - generate ieee_addr from node_id 
  if (settings_check(SETTINGS_KEY_EUI64, 0) != true || settings_get(SETTINGS_KEY_EUI64, 0, (void*) &eui64_addr, sizeof(eui64_addr)) != SETTINGS_STATUS_OK) {
#if CONTIKI_CONF_RANDOM_MAC
    generate_new_eui64(eui64_addr);
    PRINTD("Radio IEEE Addr not in EEPROM - generated random\n");
#else /* CONTIKI_CONF_RANDOM_MAC */
    eui64_addr[0] = node_id & 0xFF;
    eui64_addr[1] = (node_id >> 8) & 0xFF;
    eui64_addr[2] = 0;
    eui64_addr[3] = 0;
    eui64_addr[4] = 0;
    eui64_addr[5] = 0;
    eui64_addr[6] = 0;
    eui64_addr[7] = 0;
    PRINTD("Radio IEEE Addr not in EEPROM - using default\n");
#endif /* CONTIKI_CONF_RANDOM_MAC */
#if WRITE_EUI64
    if (settings_set(SETTINGS_KEY_EUI64, eui64_addr, sizeof (eui64_addr)) == SETTINGS_STATUS_OK) {
      PRINTD("Wrote new IEEE Addr to EEPROM.\n");
    } else {
      PRINTD("Failed writing IEEE Addr to EEPROM.\n");
    }
#endif
  }
#endif

#if EUI64_BY_NODE_ID
  /* Replace lower 2 bytes with node ID  */
  eui64_addr[0] = node_id & 0xFF;
  eui64_addr[1] = (node_id >> 8) & 0xFF;
#endif

  PRINTA("Network ID (pan_id): 0x%04X\n", pan_id);
  PRINTA("Node ID (pan_addr): 0x%04X\n", node_id);
  PRINTA("Radio TX power: 0x%02X\n", radio_tx_power);
  PRINTA("Radio channel: 0x%02X\n", radio_channel);
  PRINTA("MAC(EUI64) address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n\r",
          eui64_addr[0],
          eui64_addr[1],
          eui64_addr[2],
          eui64_addr[3],
          eui64_addr[4],
          eui64_addr[5],
          eui64_addr[6],
          eui64_addr[7]);

#if RF230BB

  /* Start radio and radio receive process */
  NETSTACK_RADIO.init();

  //--- Set Rime address based on eui64
  {
    rimeaddr_t addr;
    memcpy(addr.u8, eui64_addr, sizeof (rimeaddr_t));
    PRINTF("rime address: ");
    int i;
    for (i = 0; i < sizeof (rimeaddr_t); i++) {
      PRINTF("%02x.", addr.u8[i]);
    }
    PRINTF("\n");

    rimeaddr_set_node_addr(&addr);
  }

  //--- Radio address settings
  {
    /* change order of bytes for rf23x */
    uint16_t inv_node_id = ((node_id >> 8) & 0xff) + ((node_id & 0xff) << 8);
    rf230_set_pan_addr(
            pan_id, // Network address 2 byte
            inv_node_id, // PAN ADD 2 Byte
            eui64_addr // MAC ADDRESS 8 byte
            );

    rf230_set_channel(radio_channel);
    rf230_set_txpower(radio_tx_power);
  }

  /* Initialize stack protocols */
  queuebuf_init();
  NETSTACK_RDC.init();
  NETSTACK_MAC.init();
  NETSTACK_NETWORK.init();

  printf("%s %s, channel check rate %lu Hz, radio channel %u, power %u\n",
          NETSTACK_MAC.name, NETSTACK_RDC.name,
          CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval() == 0 ? 1 :
          NETSTACK_RDC.channel_check_interval()),
          rf230_get_channel(),
          rf230_get_txpower());

#else /* RF230BB */

  /* Original RF230 combined mac/radio driver */
  /* mac process must be started before tcpip process! */
  process_start(&mac_process, NULL);
#endif

#if UIP_CONF_IPV6
  // Copy EUI64 to the link local address
  memcpy(&uip_lladdr.addr, &eui64_addr, sizeof (uip_lladdr.addr));

  process_start(&tcpip_process, NULL);

  printf("Tentative link-local IPv6 address ");
  {
    uip_ds6_addr_t *lladdr;
    int i;
    lladdr = uip_ds6_get_link_local(-1);
    for (i = 0; i < 7; ++i) {
      printf("%02x%02x:", lladdr->ipaddr.u8[i * 2],
              lladdr->ipaddr.u8[i * 2 + 1]);
    }
    printf("%02x%02x\n", lladdr->ipaddr.u8[14], lladdr->ipaddr.u8[15]);
  }

#endif /* UIP_CONF_IPV6 */
}

/*-------------------------Low level initialization------------------------*/
/*------Done in a subroutine to keep main routine stack usage small--------*/
void
init(void)
{
  extern void *watchdog_return_addr;
  extern uint8_t mcusr_mirror;

  /* Save the address where the watchdog occurred */
  void *wdt_addr = watchdog_return_addr;
  MCUSR = 0;

  watchdog_init();
  watchdog_start();

  /* Second rs232 port for debugging */
  rs232_init(RS232_PORT_0, USART_BAUD_INGA, USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
  /* Redirect stdout to second port */
  rs232_redirect_stdout(RS232_PORT_0);

  /* wait here to get a chance to see boot screen. */
  _delay_ms(200);

  PRINTA("\n*******Booting %s*******\nReset reason: ", CONTIKI_VERSION_STRING);
  /* Print out reset reason */
  if (mcusr_mirror & _BV(JTRF))
    PRINTA("JTAG ");
  if (mcusr_mirror & _BV(WDRF))
    PRINTA("Watchdog ");
  if (mcusr_mirror & _BV(BORF))
    PRINTA("Brown-out ");
  if (mcusr_mirror & _BV(EXTRF))
    PRINTA("External ");
  if (mcusr_mirror & _BV(PORF))
    PRINTA("Power-on ");
  PRINTA("\n");
  if (mcusr_mirror & _BV(WDRF))
    PRINTA("Watchdog possibly occured at address %p\n", wdt_addr);

  clock_init();

#if STACKMONITOR
#define STACK_FREE_MARK 0x4242
  /* Simple stack pointer highwater monitor.
   * Places magic numbers in free RAM that are checked in the main loop.
   * In conjuction with PERIODICPRINTS, never-used stack will be printed
   * every STACKMONITOR seconds.
   */
  {
    extern uint16_t __bss_end;
    uint16_t p = (uint16_t) & __bss_end;
    do {
      *(uint16_t *) p = STACK_FREE_MARK;
      p += 10;
    } while (p < SP - 10); //don't overwrite our own stack
  }
#endif

  /* Get a random (or probably different) seed for the 802.15.4 packet sequence number.
   * Some layers will ignore duplicates found in a history (e.g. Contikimac)
   * causing the initial packets to be ignored after a short-cycle restart.
   */
#if CONTIKI_CONF_RANDOM_MAC
  random_init(rng_get_uint8());
#endif


  /* Flash initialization */
  at45db_init();

#ifdef MICRO_SD_PWR_PIN
  /* set pin for micro sd card power switch to output */
  MICRO_SD_PWR_PORT_DDR |= (1 << MICRO_SD_PWR_PIN);
#endif

  /* rtimers needed for radio cycling */
  rtimer_init();

  /* Initialize process subsystem */
  process_init();

  /* etimers must be started before ctimer_init */
  process_start(&etimer_process, NULL);

  ctimer_init();

#if defined(APP_SETTINGS_SET)
  process_start(&settings_set_process, NULL);
#endif

#if defined(APP_SETTINGS_DELETE)
  process_start(&settings_delete_process, NULL);
#endif    

#if PLATFORM_RADIO
  // Init radio
  platform_radio_init();
#endif

#if ANNOUNCE_BOOT
  PRINTA("%s %s, channel %u power %u", NETSTACK_MAC.name, NETSTACK_RDC.name, rf230_get_channel(), rf230_get_txpower());
  if (NETSTACK_RDC.channel_check_interval) {//function pointer is zero for sicslowmac
    unsigned short tmp;
    tmp = CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval == 0 ? 1 : \
 NETSTACK_RDC.channel_check_interval());
    if (tmp < 65535) PRINTA(", check rate %u Hz", tmp);
  }
  PRINTA("\n");

#if UIP_CONF_IPV6_RPL
  PRINTA("RPL Enabled\n");
#endif
#if UIP_CONF_ROUTER
  PRINTA("Routing Enabled, TCP_MSS: %u\n", UIP_TCP_MSS);
#endif

#endif /* ANNOUNCE_BOOT */

  PRINTA("Online\n");
}

#if PER_ROUTES
static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int8_t i, f;
  for (i = 0, f = 0; i < sizeof (uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if (a == 0 && f >= 0) {
      if (f++ == 0) PRINTF("::");
    } else {
      if (f > 0) {
        f = -1;
      } else if (i > 0) {
        PRINTF(":");
      }
      PRINTF("%04x", a);
    }
  }
}
#endif
/*---------------------------------------------------------------------------*/
#if PERIODICPRINTS
static void
periodic_prints()
{
  static uint32_t clocktime;

  if (clocktime != clock_seconds()) {
    clocktime = clock_seconds();

#if PER_STAMPS
    /* Print time stamps. */
    if ((clocktime % PER_STAMPS) == 0) {
#if ENERGEST_CONF_ON
#include "lib/print-stats.h"
      print_stats();
#elif RADIOSTATS
      extern volatile unsigned long radioontime;
      PRINTF("%u(%u)s\n", clocktime, radioontime);
#else /* RADIOSTATS */
      PRINTF("%us\n", clocktime);
#endif /* RADIOSTATS */
    }
#endif /* PER_STAMPS */

#if PER_PINGS&&0
    extern void raven_ping6(void);
    if ((clocktime % PER_PINGS) == 1) {
      PRINTF("**Ping\n");
      raven_ping6();
    }
#endif /* PER_PINGS */

#if PER_ROUTES
    if ((clocktime % PER_ROUTES) == 2) {

      extern uip_ds6_nbr_t uip_ds6_nbr_cache[];
      extern uip_ds6_route_t uip_ds6_routing_table[];
      extern uip_ds6_netif_t uip_ds6_if;

      uint8_t i, j;
      PRINTF("\nAddresses [%u max]\n", UIP_DS6_ADDR_NB);
      for (i = 0; i < UIP_DS6_ADDR_NB; i++) {
        if (uip_ds6_if.addr_list[i].isused) {
          ipaddr_add(&uip_ds6_if.addr_list[i].ipaddr);
          PRINTF("\n");
        }
      }
      PRINTF("\nNeighbors [%u max]\n", UIP_DS6_NBR_NB);
      for (i = 0, j = 1; i < UIP_DS6_NBR_NB; i++) {
        if (uip_ds6_nbr_cache[i].isused) {
          ipaddr_add(&uip_ds6_nbr_cache[i].ipaddr);
          PRINTF("\n");
          j = 0;
        }
      }
      if (j) PRINTF("  <none>");
      PRINTF("\nRoutes [%u max]\n", UIP_DS6_ROUTE_NB);
      for (i = 0, j = 1; i < UIP_DS6_ROUTE_NB; i++) {
        if (uip_ds6_routing_table[i].isused) {
          ipaddr_add(&uip_ds6_routing_table[i].ipaddr);
          PRINTF("/%u (via ", uip_ds6_routing_table[i].length);
          ipaddr_add(&uip_ds6_routing_table[i].nexthop);
          //     if(uip_ds6_routing_table[i].state.lifetime < 600) {
          PRINTF(") %lus\n", uip_ds6_routing_table[i].state.lifetime);
          //     } else {
          //       PRINTF(")\n");
          //     }
          j = 0;
        }
      }
      if (j) PRINTF("  <none>");
      PRINTF("\n---------\n");
    }
#endif /* PER_ROUTES */

#if STACKMONITOR
    /* Checks for highest address with STACK_FREE_MARKs in RAM */
    if ((clocktime % STACKMONITOR) == 3) {
      extern uint16_t __bss_end;
      uint16_t p = (uint16_t) & __bss_end;
      do {
        if (*(uint16_t *) p != STACK_FREE_MARK) {
          PRINTF("Never-used stack > %d bytes\n", p - (uint16_t) & __bss_end);
          break;
        }
        p += 10;
      } while (p < RAMEND - 10);
    }
#endif /* STACKMONITOR */
  }
}
#endif /* PERIODICPRINTS */
/*-------------------------------------------------------------------------*/
/*------------------------- Main Scheduler loop----------------------------*/
/*-------------------------------------------------------------------------*/

// setup sensors
SENSORS(&button_sensor, &acc_sensor, &gyro_sensor, &pressure_sensor, &battery_sensor);

int
main(void)
{
  init();
  /* Start sensor init process */
  process_start(&sensors_process, NULL);
  /* Autostart other processes */
  autostart_start(autostart_processes);

  while (1) {
    process_run();
    watchdog_periodic();

#if DEBUGFLOWSIZE
    if (debugflowsize) {
      debugflow[debugflowsize] = 0;
      PRINTF("%s", debugflow);
      debugflowsize = 0;
    }
#endif

    watchdog_periodic();

#if PERIODICPRINTS
    periodic_prints();
#endif /* PERIODICPRINTS */

  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* implements sys/log interface */
void
log_message(char *m1, char *m2)
{
  PRINTF("%s%s\n", m1, m2);
}
