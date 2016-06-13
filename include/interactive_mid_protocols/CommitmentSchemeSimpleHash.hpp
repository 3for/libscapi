#pragma once
#include "CommitmentScheme.hpp"
#include "../comm/Comm.hpp"
#include "../../include/primitives/HashOpenSSL.hpp"

/**
* This class holds the values used by the SimpleHash Committer during the commitment phase
* for a specific value that the committer commits about.
* This value is kept attached to a random value used to calculate the commitment,
* which is also kept together in this structure.
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtSimpleHashCommitmentValues :public CmtCommitmentPhaseValues {
private:
	//The value that the committer sends to the receiver in order to commit commitval in the commitment phase.
	shared_ptr<vector<byte>> computedCommitment;

public:
	/**
	* Constructor that sets the given random value, committed value and the commitment object.
	* This constructor is package private. It should only be used by the classes in the package.
	* @param r random value used for commit.
	* @param commitVal the committed value
	* @param computedCommitment the commitment
	*/
	CmtSimpleHashCommitmentValues(shared_ptr<RandomValue> r, shared_ptr<CmtCommitValue> commitVal, shared_ptr<vector<byte>> computedCommitment) 
		: CmtCommitmentPhaseValues(r, commitVal) {
		this->computedCommitment = computedCommitment;
	}

	shared_ptr<void> getComputedCommitment() override { return computedCommitment; };
		
};

/**
* Concrete implementation of commitment message used by SimpleHash commitment scheme.
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtSimpleHashCommitmentMessage : public CmtCCommitmentMsg {

private:
	// In SimpleHash schemes the commitment object is a vector<byte>. 
	shared_ptr<vector<byte>> c;
	long id; //The id of the commitment

public:
	/**
	* Constructor that sets the commitment and id.
	* @param c the actual commitment object. In simple hash schemes the commitment object is a byte[].
	* @param id the commitment id.
	*/
	CmtSimpleHashCommitmentMessage(shared_ptr<vector<byte>> c = NULL, long id = 0) {
		this->c = c;
		this->id = id;
	}

	/**
	* Returns the commitment value
	*/
	shared_ptr<void> getCommitment() override {	return c; }

	/**
	* Returns the commitment id.
	*/
	long getId() override { return id; };

	// network serialization implementation:
	void initFromString(const string & s) override;
	string toString() override;
};

/**
* Concrete implementation of decommitment message used by SimpleHash commitment scheme.
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtSimpleHashDecommitmentMessage : public CmtCDecommitmentMessage {
private:
	shared_ptr<ByteArrayRandomValue> r; //Random value sampled during the commitment stage;
	vector<byte> x; //Committer's private input x 
	
public:
	CmtSimpleHashDecommitmentMessage() {}

	/**
	* Constructor that sets the given committed value and random value.
	* @param x the committed value
	* @param r the random value used for commit.
	*/
	CmtSimpleHashDecommitmentMessage(shared_ptr<ByteArrayRandomValue> r, vector<byte> x) {
		this->r = r;
		this->x = x;
	}

	vector<byte> getX() { return x;	}

	shared_ptr<RandomValue> getR() override { return r;	}

	// network serialization implementation:
	void initFromString(const string & s) override;
	string toString() override;

};

/**
* This class implements the committer side of Simple Hash commitment.<p>
*
* This is a commitment scheme based on hash functions. <p>
* It can be viewed as a random-oracle scheme, but its security can also be viewed as a
* standard assumption on modern hash functions. Note that computational binding follows
* from the standard collision resistance assumption. <p>
*
* The pseudo code of this protocol can be found in Protocol 3.6 of pseudo codes document at {@link http://cryptobiu.github.io/scapi/SDK_Pseudocode.pdf}.<p>
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtSimpleHashCommitter : public CmtCommitter, public SecureCommit, public CmtOnByteArray {

	/*
	* runs the following protocol:
	* "Commit phase
	*		SAMPLE a random value r <- {0, 1}^n
	*		COMPUTE c = H(r,x) (c concatenated with r)
	*		SEND c to R
	*	Decommit phase
	*		SEND (r, x)  to R
	*		OUTPUT nothing"
	*/

private:
	shared_ptr<CryptographicHash> hash;
	int n;
	mt19937 random;
	
	void doConstruct(shared_ptr<CommParty> channel, shared_ptr<CryptographicHash> hash, int n = 32);

	/**
	* Computes the hash function on the concatination of the inputs.
	* @param x user input
	* @param r random value
	* @return the hash result.
	*/
	shared_ptr<vector<byte>> computeCommitment(vector<byte> x, vector<byte> r);

public:
	/**
	* Constructor that receives a connected channel (to the receiver) and chosses default
	* values for the hash function, SecureRandom object and a security parameter n.
	*  @param channel
	*/
	CmtSimpleHashCommitter(shared_ptr<CommParty> channel);

	/**
	* Constructor that receives a connected channel (to the receiver), the hash function
	* agreed upon between them, a SecureRandom object and a security parameter n.
	* The Receiver needs to be instantiated with the same hash, otherwise nothing will work properly.
	* @param channel
	* @param hash
	* @param random
	* @param n security parameter
	*
	*/
	CmtSimpleHashCommitter(shared_ptr<CommParty> channel, shared_ptr<CryptographicHash> hash, int n = 32) {
		doConstruct(channel, hash, n);
	}

	/**
	* Runs the following lines of the commitment scheme:
	* "SAMPLE a random value r <- {0, 1}^n
	*	COMPUTE c = H(r,x) (c concatenated with r)".
	* @return the generated commitment.
	*
	*/
	shared_ptr<CmtCCommitmentMsg> generateCommitmentMsg(shared_ptr<CmtCommitValue> input, long id) override; 

	shared_ptr<CmtCDecommitmentMessage> generateDecommitmentMsg(long id) override;

	/**
	* This function samples random commit value and returns it.
	* @return the sampled commit value
	*/
	shared_ptr<CmtCommitValue> sampleRandomCommitValue() override;

	shared_ptr<CmtCommitValue> generateCommitValue(vector<byte> x) override {
		return make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(x));
	}

	/**
	* No pre-process is performed for Simple Hash Committer, therefore this function
	* returns empty vector.
	*/
	vector<shared_ptr<void>> getPreProcessValues() override { 
		vector<shared_ptr<void>> empty;
		return empty;
	}

	/**
	* This function converts the given commit value to a byte array.
	* @param value
	* @return the generated bytes.
	*/
	vector<byte> generateBytesFromCommitValue(CmtCommitValue* value) override; 
};

/**
* This class implements the receiver side of Simple Hash commitment.<p>
*
* This is a commitment scheme based on hash functions. <p>
* It can be viewed as a random-oracle scheme, but its security can also be viewed as a standard assumption on modern hash functions.
* Note that computational binding follows from the standard collision resistance assumption. <p>
*
* The pseudo code of this protocol can be found in Protocol 3.6 of pseudo codes document at {@link http://cryptobiu.github.io/scapi/SDK_Pseudocode.pdf}.<p>
*
* @author Cryptography and Computer Security Research Group Department of Computer Science Bar-Ilan University (Yael Ejgenberg)
*
*/
class CmtSimpleHashReceiver : public CmtReceiver, public SecureCommit, public CmtOnByteArray {

	/*
	* runs the following protocol:
	* "Commit phase
	*		WAIT for a value c
	*		STORE c
	*	Decommit phase
	*		WAIT for (r, x)  from C
	*		IF NOT
	*			c = H(r,x), AND
	*			x <- {0, 1}^t
	*		      OUTPUT REJ
	*		ELSE
	*		      OUTPUT ACC and value x"
	*/

private:
	
	shared_ptr<CommParty> channel;
	shared_ptr<CryptographicHash> hash;
	int n; //security parameter.

	void doConstruct(shared_ptr<CommParty> channel, shared_ptr<CryptographicHash> hash, int n = 32);

public:
	/**
	* Constructor that receives a connected channel (to the receiver) and chosses default
	* values for the hash function, SecureRandom object and a security parameter n.
	*  @param channel
	*/
	CmtSimpleHashReceiver(shared_ptr<CommParty> channel) {
		doConstruct(channel, make_shared<OpenSSLSHA256>());
	}


	/**
	* Constructor that receives a connected channel (to the receiver), the hash function
	* agreed upon between them and a security parameter n.
	* The committer needs to be instantiated with the same DlogGroup, otherwise nothing will work properly.
	* @param channel
	* @param hash
	* @param n security parameter
	*
	*/
	CmtSimpleHashReceiver(shared_ptr<CommParty> channel, shared_ptr<CryptographicHash> hash, int n = 32) {
		doConstruct(channel, hash, n);
	}

	/**
	* Run the commit phase of the protocol:
	* "WAIT for a value c
	*	STORE c".
	*/
	shared_ptr<CmtRCommitPhaseOutput> receiveCommitment() override; 

	/**
	* Run the decommit phase of the protocol:
	* "WAIT for (r, x)  from C
	*	IF NOT
	*		c = H(r,x), AND
	*		x <- {0, 1}^t
	*		OUTPUT REJ
	*	ELSE
	*	  	OUTPUT ACC and value x".
	*/
	shared_ptr<CmtCommitValue> receiveDecommitment(long id) override; 

	shared_ptr<CmtCommitValue> verifyDecommitment(CmtCCommitmentMsg* commitmentMsg,	CmtCDecommitmentMessage* decommitmentMsg) override; 

	/**
	* No pre-process is performed for Simple Hash Receiver, therefore this function returns null!
	*/
	vector<shared_ptr<void>> getPreProcessedValues() override {
		vector<shared_ptr<void>> empty;
		return empty;
	}	

	/**
	* This function converts the given commit value to a byte array.
	* @param value
	* @return the generated bytes.
	*/
	vector<byte> generateBytesFromCommitValue(CmtCommitValue* value) override; 
};

