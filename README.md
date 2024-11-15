This is the artifact for "Re-visiting Authorized Private Set Intersection: A New Privacy-Preserving Variant and Two Protocols" by Francesca Falzon and Evangelia Anna Markatou (PoPETS 2025). This respository builds upon the [MCL Pairings Library](https://github.com/herumi/mcl/tree/master). 

In this respository, you will find implementations and tests for three protocols:

1. The Authorized Private Set Intersection (APSI) protocol from Falzon and Markatou (PoPETS 2025)
2. The Parital-APSI protocol from Falzon and Markatou (PoPETS 2025)
3. The APSI protocol from De Cristofaro and Tsudik (Financial Crypto 2010)

The third protocol is re-implemented for comparison. 

# Getting Started

First make sure that the [GMP Library](https://gmplib.org/) is installed; this library is necessary for the MCL pairings operations. 
GMP can be installed using the following terminal commands:

    sudo apt install libgmp-dev (on Ubuntu)
    brew install gmp (on MacOS)

Then clone this respository and make the following edits to the files described below, as needed:

- To build for **Mac M1**, simply navigate to the `c_code` directory.

- To build for **Mac M2**, go into `c_code` and edit the Makefile to comment out the Mac M1 flags (Lines 38-41) and 
Linux flags (Lines 52-53), and uncomment the Mac M2 ones (Lines 44-49). Then go to common.mk and comment out the path to GMP for Mac M1/Linux (line 139) and uncomment the path for M2 (line 140).

- To build for **Linux**, go into `c_code` and edit the Makefile to comment out the Mac M1 flags (Lines 38-41) and Mac M2 flags (Lines 44-49), and uncomment the Linux flags (Lines 52-53). Then go to common.mk and ensure that the path to GMP for Mac M1/Linux (line 139) is un commented, and that the path for M2 is commented out (line 140). 

Then run:

    make clean
    make sample

# Running the APSI/Partial-APSI Protocol

To run our APSI or Partial-APSI protocol, use the following command:

    bin/automate.exe {apsi, papsi} {credit_cards, random} lenght_of_random_string n m outputfile p

Here, `credit_cards` generates random 16 digit strings, and `random` generates random strings of length specified by `length_of_random_string` (note that the `length_of_random_string` parameter must be added regardless of whether the credit card or random option is chosen; however, in the case of the credit card option, the parameter is simply ignored). Parameters `n` and `m` denote the sizes of the client and server sets, respectively. The `outputfile` is the name of the file to which the results are written to. Laslty, `p` is a percent from 1 to 100 denoting the percent of client values revealed to judge in Partial-APSI (again, this parameter must be added regardless of whether `apsi` or `papsi` is chosen, but in the case of APSI, the parameter is simply ignored). 

Example for APSI with a random dataset:

    bin/automate.exe apsi random 4 1000 1000 "results.txt" 100

Example for Partial-APSI with a credit card dataset:

    bin/automate.exe apsi credit_cards 0 1000 1000 "results.txt" 50


# Running the DT10 Protocol

To run the APSI protocol from "Practical Private Set Intersection Protocols with Linear Computational and Bandwidth Complexity" by Emiliano De Cristofaro and Gene Tsudik (FC 2010), use the following command:

    bin/dt10.exe dt10 {credit_cards, random} lenght_of_random_string n m outputfile

Example:

    bin/dt10.exe dt10 random 4 1000 1000 "results.txt" 100

