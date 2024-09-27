
#include <mcl/bn256.hpp>

#include <fstream>
#include <iostream>
#include <tuple>
static cybozu::RandomGenerator rg;
using namespace mcl::bn;

#include <mcl/lagrange.hpp>
#include <tuple>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <cstdlib>

#include <chrono>
#include <execution>
#include <algorithm>


using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;




using namespace std;


set<string> generate_credit_cards(int number_of_credit_cards)
{   
    set<string> credit_cards;
    
    int i, j;
    string str;
    char c;

    for(i=0; i<number_of_credit_cards; i++)
    {
        for (j=0; j<16; j++)
        {
            c = '0' + rand() % 10;
            str += c;
        }
        credit_cards.insert(str);
        str.clear();
    }
    return credit_cards;
}


// https://inversepalindrome.com/blog/how-to-create-a-random-string-in-cpp
string random_string(size_t length)
{
    const string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    random_device random_device;
    mt19937 generator(random_device());
    uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

    string random_string;

    for (size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

int set_intersection_strings(set<string> xs, set<string> ys) {
    set<string> common_elements; 
    set_intersection(xs.begin(), xs.end(), ys.begin(), 
                     ys.end(), 
                     inserter(common_elements, common_elements.begin())); 

	return common_elements.size();
}




void Hash(G1& P, const string& m)
{
	Fp t;
	t.setHashOf(m);
	mapToG1(P, t);
}

void KeyGen(Fr& s, G2& pub, const G2& Q)
{
	s.setHashOf(random_string(32));
	G2::mul(pub, Q, s); // pub = sQ
}

void Sign(G1& sign, const Fr& s, const string& m)
{
	G1 Hm;
	Hash(Hm, m);
	G1::mul(sign, Hm, s); // sign = s H(m)
}

bool Verify(const G1& sign, const G2& Q, const G2& pub, const string& m)
{
	Fp12 e1, e2;
	G1 Hm;
	Hash(Hm, m);
	pairing(e1, sign, Q); // e1 = e(sign, Q)
	pairing(e2, Hm, pub); // e2 = e(Hm, sQ)
	return e1 == e2;
}


G1 judge_sign_apsi(string x, Fr sk, string client_id) {
	G1 sign;
	x = x + client_id;
	Sign(sign, sk, x);
	return sign;
}




Fp12 server_y_hats_apsi(string y, G2 pks, Fr s) {

	Fp12 y_hat;
	G1 Hy;
	Hash(Hy, y);
	pairing(y_hat, Hy, pks);
	return y_hat;

}





// APSI 
string APSI(set<string> xs, set<string> ys, int correct_inter)
{
	cout << "This is APSI." << endl;
	string client_id = "123456789";
	string times;
	int server_time=0;
	int client_time=0;
	int judge_time=0;

	int client_inter_time=0;
	int client_auth_time=0;

	int authorize_time=0;
	int intersect_time=0;

	long long authorize_com=0;
	long long intersect_com=0;

	// setup parameter
	initPairing();
	G2 Q;
	mapToG2(Q, 1);

	auto t1 = high_resolution_clock::now();
	// generate secret key and public key
	Fr sk;
	G2 pk;
	KeyGen(sk, pk, Q);
	authorize_com += 2*sizeof(sk) + 2*sizeof(pk);

	// Need vector for parallelization 
	vector<string> new_xs;
	for (string x: xs) {
		new_xs.push_back(x);
	}
	vector<G1> signatures(new_xs.size());

	authorize_com += new_xs.size()*sizeof(new_xs[0]);


	

	// Judge signing
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		G1 sign = judge_sign_apsi(new_xs[i], sk, client_id);
		signatures[i] = sign;
	}

	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Judge signing takes " <<  ms_int.count() << "ms\n";
	times = times + to_string(ms_int.count()) + " ";

	judge_time += ms_int.count();
	authorize_time += ms_int.count();

	authorize_com += signatures.size()*sizeof(signatures[0]);





	// Server 
	t1 = high_resolution_clock::now();
	Fr s;
	G2 S;
	KeyGen(s, S, Q);
	G2 pks;
	G2::mul(pks, pk, s);

	intersect_com += sizeof(S);

	// Need vector for parallelization 
	vector<string> new_ys;
	for (string y: ys) {
		new_ys.push_back(y);
	}

	// Parallelized y_hats (pairings) 
	#pragma omp parallel for 
	for (int i=0; i<new_ys.size(); i++) {
		string y_c = new_ys[i] + client_id;
		Fp12 y_hat = server_y_hats_apsi(y_c, pks, s);
		new_ys[i] = y_hat.getStr(256).c_str();
	}

	// Back to sets for intersection
	set<string> y_hats_set;
	for (string y: new_ys) {
		y_hats_set.insert(y);
	}

	intersect_com += new_ys.size()*sizeof(new_ys[0]);


	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Server takes " <<  ms_int.count() << "ms\n";

	server_time += ms_int.count();
	intersect_time += ms_int.count();

	// Back to client
	t1 = high_resolution_clock::now();
	vector<string> x_hats(signatures.size());

	// Parallelized x_hats (pairings) 
	#pragma omp parallel for 
	for (int i=0; i<signatures.size(); i++) {
		Fp12 x_hat;
		pairing(x_hat, signatures[i], S);
		x_hats[i] = x_hat.getStr(256).c_str();
	}

	// Back to sets for intersection
	set<string> x_hats_set;
	for (string x: x_hats) {
		x_hats_set.insert(x);
	}




	int inter_size = set_intersection_strings(x_hats_set,y_hats_set);
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Client takes " <<  ms_int.count() << "ms\n";
	times = times + to_string(ms_int.count());

	client_time += ms_int.count();
	client_inter_time +=  ms_int.count();
	intersect_time += ms_int.count();

	return to_string(authorize_com)+ " & " + to_string(intersect_com) + " & " + to_string(judge_time) + " & " +  to_string(client_auth_time) + " & " + to_string(client_inter_time) + " & " + to_string(server_time);
}

// PAPSI 
string PAPSI(set<string> xs, set<string> ys, int correct_inter, int p)
{
	cout << "This is PAPSI." << endl;
	// setup parameter
	initPairing();
	G2 Q;
	mapToG2(Q, 1);
	string client_id = "123456789";
	string times;


	int server_time=0;
	int client_time=0;
	int judge_time=0;

	int client_inter_time=0;
	int client_auth_time=0;

	int authorize_time=0;
	int intersect_time=0;

	long long authorize_com=0;
	long long intersect_com=0;


	auto t1 = high_resolution_clock::now();
	// generate secret key and public key
	Fr sk;
	G2 pk;
	KeyGen(sk, pk, Q);

	authorize_com += 2*sizeof(pk) + 2*sizeof(sk);

	// Picking I 
	set<int> I;
	for (int i=0; i<xs.size(); i++) {
		if ( rand() % 100 < p) {
			I.insert(i);
		}
	}

	int secret = 42;

	authorize_com += I.size()*sizeof(secret);


	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Picking I and pk,sk " <<  ms_int.count() << "ms\n";

	judge_time += ms_int.count();
	authorize_time += ms_int.count();


	t1 = high_resolution_clock::now();
	Fr inv_r;
	const Fr r = rand();
	mcl::bn::Fr::inv(inv_r, r);

	vector<G1> blind_xs(xs.size());

	// Need vector for parallelization 
	vector<string> new_xs;
	for (string x: xs) {
		new_xs.push_back(x);
	}

	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		// Get x message 
		string x = new_xs[i] + client_id;

		// Blind message 
		G1 Hx;
		Hash(Hx, x);
		G1 Hxr;
		G1::mul(Hxr, Hx, r); // This is X
		blind_xs[i] = Hxr;
	}

	authorize_com += blind_xs.size()*sizeof(blind_xs[0]);


	// proving function  
	const Fr random_eea = rand();
	int c_eea;
	vector<G1> tis(xs.size());
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		if (I.contains(i)) {
			// Get x message 
			string x = new_xs[i] + client_id;

			// Blind message 
			G1 Hx;
			Hash(Hx, x);
			G1 Hxr;
			G1::mul(Hxr, Hx, random_eea); // This is t_i
			tis[i] = Hxr;
		}
	}

	authorize_com += tis.size()*sizeof(tis[0]);

	
	hash<string> h;
	for (int i=0; i<new_xs.size(); i++) {
		string long_str = tis[i].serializeToHexStr().c_str();
		long_str += new_xs[i];

		c_eea = h(long_str+to_string(c_eea));
		//c_eea = c_eea % 100000000;
	}

	Fr c_tmp = c_eea;
	Fr s_eea = random_eea + c_tmp*r; 

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "The client blinding the elements + proof " <<  ms_int.count() << "ms\n";

	client_time += ms_int.count();
	client_auth_time += ms_int.count();
	authorize_time += ms_int.count();





	t1 = high_resolution_clock::now();


	// verify function  
	#pragma omp parallel for 
	for (int i=0; i<new_xs.size(); i++) {
		if (I.contains(i)) {
			string x_eea = new_xs[i] + client_id;
			G1 Hx_eea;
			Hash(Hx_eea, x_eea);

			G1 right; 
			G1::mul(right, Hx_eea, s_eea); 

			G1 check;
			G1::mul(check, blind_xs[i], c_tmp); 


			G1 left = tis[i] + check;
			bool correct;
			correct = left.serializeToHexStr()==right.serializeToHexStr();

			if (!correct) {
				cout << "Verification fails for item " << i << "\n";
			}
		//cout << (correct ? "yes!" : "noooooooo :(") << "\n";
		}
	}


	vector<G1> signatures(blind_xs.size());
	#pragma omp parallel for 
	for (int i=0; i<blind_xs.size(); i++) {
		// sign
		G1 sign;
		G1::mul(sign, blind_xs[i], sk); 
		signatures[i] = sign;
	}

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Judge verifying and signing takes " <<  ms_int.count() << "ms\n";

	judge_time += ms_int.count();
	authorize_time += ms_int.count();

	authorize_com += signatures.size()*sizeof(signatures[0]);


	// Server 

	t1 = high_resolution_clock::now();
	Fr s;
	s.setHashOf(random_string(32));
	G2 S;
	G2 pks;
	KeyGen(s, S, Q);
	G2::mul(pks, pk, s);
	intersect_com += sizeof(S);
	

	//OPRF
	Fr inv_t;
	const Fr t = rand();
	mcl::bn::Fr::inv(inv_t, t);
	int oprf_time = 0;

	// Need vector for parallelization 
	vector<string> new_ys;
	for (string y: ys) {
		new_ys.push_back(y);
	}


	// OPRF Request 
	vector<G1> oprf_vector(new_ys.size());
	#pragma omp parallel for 
	for (int i=0; i<new_ys.size(); i++) {
		string y = new_ys[i] + client_id;
		G1 Hy;
		Hash(Hy, y);
		G1 Hyt;
		G1::mul(Hyt, Hy, t);
		oprf_vector[i] = Hyt;
	}

	intersect_com += oprf_vector.size()*sizeof(oprf_vector[0]);

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "OPRF Request takes " <<  ms_int.count() << "ms\n";

	server_time = ms_int.count();
	intersect_time += ms_int.count();

	// OPRF Eval 
	t1 = high_resolution_clock::now();
	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		G1 Hytr;
		G1::mul(Hytr, oprf_vector[i], r);
		oprf_vector[i] = Hytr;
	}
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "OPRF Eval takes " <<  ms_int.count() << "ms\n";

	client_time += ms_int.count();
	client_inter_time += ms_int.count();
	intersect_time += ms_int.count();

	intersect_com += oprf_vector.size()*sizeof(oprf_vector[0]);


	// OPRF Recover 
	t1 = high_resolution_clock::now();
	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		G1 Hytrinv_t;
		G1::mul(Hytrinv_t, oprf_vector[i], inv_t);
		oprf_vector[i] = Hytrinv_t;
	}
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "OPRF Recover takes " <<  ms_int.count() << "ms\n";

	server_time += ms_int.count();
	intersect_time += ms_int.count();


	// Making yhats
	t1 = high_resolution_clock::now();
	vector<string> y_hats(oprf_vector.size());

	#pragma omp parallel for 
	for (int i=0; i<oprf_vector.size(); i++) {
		Fp12 y_hat;
		pairing(y_hat, oprf_vector[i], pks);
		y_hats[i] = y_hat.getStr(256).c_str();
	}

	intersect_com += y_hats.size()*sizeof(y_hats[0]);

	// Back to sets for intersection
	set<string> y_hats_set;
	for (string y: y_hats) {
		y_hats_set.insert(y);
	}

	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Server Making yhats takes " <<  ms_int.count() << "ms\n";
	times = times + to_string(ms_int.count()) + " ";

	server_time += ms_int.count();
	intersect_time += ms_int.count();

	// Client 
	t1 = high_resolution_clock::now();
	vector<string> x_hats(signatures.size());

	#pragma omp parallel for 
	for (int i=0; i<signatures.size(); i++) {
		Fp12 x_hat;
		pairing(x_hat, signatures[i], S);
		x_hats[i] = x_hat.getStr(256).c_str();
	}

	// Back to sets for intersection
	set<string> x_hats_set;
	for (string x: x_hats) {
		x_hats_set.insert(x);
	}


	int inter_size = set_intersection_strings(x_hats_set,y_hats_set);
	t2 = high_resolution_clock::now();
	ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Client xhats + intersection takes " <<  ms_int.count() << "ms\n";

	
	client_time += ms_int.count();
	client_inter_time += ms_int.count();
	intersect_time += ms_int.count();

	return to_string(authorize_com)+ " & " + to_string(intersect_com) + " & " + to_string(judge_time) + " & " +  to_string(client_auth_time) + " & " + to_string(client_inter_time) + " & " + to_string(server_time);
}




int main(int argc, char *argv[]) {
	std::string protocol = argv[1];
	std::string data_type = argv[2];
	int size_of_elements = atoi(argv[3]);
	int n = atoi(argv[4]);
	int m = atoi(argv[5]);
	std::string filename = argv[6];
	srand (time(NULL));

	// p is an int from 1-100.
	int p = atoi(argv[7]);

	string times;

	//times = times + argv[3] + " " + argv[4] + " " + argv[5] + " ";

	set<string> xs;


	cout << "Getting x " << "\n";
	if (data_type == "credit_cards") {
		xs = generate_credit_cards(n);
	} else {
		while (xs.size() < n) {
			xs.insert(random_string(size_of_elements));
		}
	}


	cout << "Getting y " << "\n";
	set<string> ys;
	if (data_type == "credit_cards") {
		ys = generate_credit_cards(m);
	} else {
		while (ys.size() < m) {
			ys.insert(random_string(size_of_elements));
		}
	}

    auto t1 = high_resolution_clock::now();
    cout << "Computing ideal functionality " << "\n";
	int correct_inter = set_intersection_strings(xs,ys);
	cout << "Correct intersection size = " << correct_inter << endl;
	auto t2 = high_resolution_clock::now();
	auto ms_int = duration_cast<milliseconds>(t2 - t1);
	std::cout << "Ideal functionality takes " <<  ms_int.count() << "ms\n";

	
	//times = times + to_string(ms_int.count()) + " " ;

	if (protocol == "apsi") {
		t1 = high_resolution_clock::now();
		times = times + APSI(xs, ys, correct_inter);
		t2 = high_resolution_clock::now();
		ms_int = duration_cast<milliseconds>(t2 - t1);
		std::cout << "APSI in total takes " <<  ms_int.count() << "ms\n";
	} else if (protocol == "papsi") {
		t1 = high_resolution_clock::now();
		times = times + PAPSI(xs, ys, correct_inter, p);
		t2 = high_resolution_clock::now();
		ms_int = duration_cast<milliseconds>(t2 - t1);
		std::cout << "PAPSI in total takes " <<  ms_int.count() << "ms\n";
	} else {
		cout << "I don't know that protocol. Pick apsi or papsi." << endl;
	}

	//cout << times << "\n";

	string result_string =  times  + "\n";


	ofstream myfile;
	myfile.open(filename, std::ios_base::app);
  	myfile << result_string;
  	myfile.close();

}