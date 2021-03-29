// makes a generic payment to r3Worx88zJEckYcifVdqMvucLS7PNBP76N from root account
const process = require('process')
const RippleAPI = require('ripple-lib').RippleAPI;
const fs = require('fs');
const api = new RippleAPI({
    server: 'ws://localhost:6005'
});
api.on('error', (errorCode, errorMessage) => {
  console.log(errorCode + ': ' + errorMessage);
});
api.on('connected', () => {
  console.log('connected');
});
api.on('disconnected', (code) => {
    console.log('disconnected, code:', code);
});

var amount = 1;
if (process.argv.length >= 3)
    amount = parseInt(process.argv[2])

api.connect().then(() => {
    j = {
        Account: 'rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh',
        TransactionType: "Payment",
        Amount: {
            currency: "USD",
            value: amount + '',
            issuer: "rECE33X6yXqM7MpjXCqG8nsdSWtSFzeGrS"
        },
        Destination: "rECE33X6yXqM7MpjXCqG8nsdSWtSFzeGrS",
        Fee: "100000"
    }
    api.prepareTransaction(j).then( (x)=> 
    {
        s = api.sign(x.txJSON, 'snoPBrXtMeMyMHUVTgbuqAfg1SUTb')
        console.log(s)
        api.submit(s.signedTransaction).then( response => {
            console.log(response.resultCode, response.resultMessage)
            console.log("Done! Paid "+amount+" USD to Hook")
            process.exit()  
        }).catch ( e=> { console.log(e) } );
     });
}).then(() => {
}).catch(console.error);