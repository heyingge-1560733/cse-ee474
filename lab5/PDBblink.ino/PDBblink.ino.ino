// Yueyang Cheng(1533989)
// lab#5 part 2
uint8_t ledOn = 0;

//this code is designed to initialize the PDB
// we display the frequency of PDB by LED

/*
  PDB_SC_TRGSEL(15)        Select software trigger
 PDB_SC_PDBEN             PDB enable
 PDB_SC_PDBIE             Interrupt enable
 PDB_SC_CONT              Continuous mode
 PDB_SC_PRESCALER(7)      Prescaler = 128
 PDB_SC_MULT(1)           Prescaler multiplication factor = 10
 */

 #define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_PDBIE | PDB_SC_CONT | PDB_SC_PRESCALER(7) | PDB_SC_MULT(1))

void setup() {
  pinMode(13,OUTPUT);
  //Enable the PDB clock
  SIM_SCGC6 = SIM_SCGC6 | SIM_SCGC6_PDB;  
  //Modulus Register, 1/(48 MHz / 128 / 10) * 37500 = 1 s
  PDB0_MOD = 37500 / 120.0;  //1 = 9.36khz  
  PDB0_IDLY = 0; //Interrupt delay
  PDB0_SC = PDB_CONFIG;
  //Software trigger (reset and restart counter)
  PDB0_SC |= PDB_SC_SWTRIG;
  //Load OK   
  PDB0_SC |= PDB_SC_LDOK;
  //Enable interrupt request  
  NVIC_ENABLE_IRQ(IRQ_PDB);
}

void loop() {
  // do nothing here
}

void pdb_isr() {
   digitalWrite(13, (ledOn = !ledOn));// invert the value of the LED each interrupt
   PDB0_SC = PDB_CONFIG | PDB_SC_LDOK; // (also clears interrupt flag)
}

