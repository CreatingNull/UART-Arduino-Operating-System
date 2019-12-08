//Description: Purely a file that handles the API of the COMMs handler

//executes the pending instruction and updates system memory where required
bool handle_comms() {
  switch(instruction_address) {
    case 64: //raw digital IO override.
      if ((instruction_len % 3) != 0) { return false; } //the data is wrong
      for (int i = 0; i < instruction_len/3; i++) {
         if (!write_IO(
          instruction_payload[uint8_t(i*3)],
          instruction_payload[uint8_t(i*3+2)],
          instruction_payload[uint8_t(i*3+1)])) { return false; } //the system cant process the action
      }
      return true; //all actions completed successfully
    case 65: //digital IO read
      return daq_instruction(1);
    case 68: //reset IO from RAM
      if (instruction_len == 0) {
         return reinit_io_from_ram();
      }
      return false;
    case 85: //Sample Analog IO
        return daq_instruction(0);
    }
  return false;
}


//Data Aq function, handling commands that have a return payload. Types: 0 = Analogue Read, Type 1 = Digital Read.
bool daq_instruction(uint8_t daq_type) {
    uint8_t response_payload_len = instruction_len;
    if (daq_type==0) { //two byte result
        response_payload_len = response_payload_len*2;
    }
    byte payload[response_payload_len];
    switch(daq_type) { //populate the payload from the system
        case 0: //Analogue read
          for (int i=0; i < instruction_len; i++) {
            if (instruction_payload[i] < 8) { // form payload 2*packet len bytes in little endian
              A_PIN_STATES[instruction_payload[i]] = read_IO(instruction_payload[i], 1);
              payload[2*i] = lowByte(A_PIN_STATES[instruction_payload[i]]);
              payload[2*i+1] = highByte(A_PIN_STATES[instruction_payload[i]]);
            }
            else {
              payload[2*i] = 255;
              payload[2*i+1] = 255;
            }
          }
          break;
        case 1: //Digital read
          for (int i = 0; i < instruction_len; i++) {
            payload[i] = read_IO(instruction_payload[i], 0);
          }
        break;
    }
    //ack is already sent so fire off the response packet now
    uint8_t ret_len = comms.generate_packet_data(payload, response_payload_len, 0, instruction_address, comms.packet_tx);
    return comms.send_packet(comms.packet_tx, ret_len);
}