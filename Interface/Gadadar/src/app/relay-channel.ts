export class RelayChannel {
    constructor(
        public id: number,
        public rlyCtrlMd: number,
        public dtCyc: number,
        public dtRng: number,
        public rlyActDT: number,
        public rlyActDr: number,
        public rlyActIT: number,
        public rlyActITOn: number,
        public state: number
      ) {  }
}
