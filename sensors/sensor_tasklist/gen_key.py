from Crypto.PublicKey import RSA

def main():
    '''
    Generate private/public key pair for wrapper/api comms
    '''

    key = RSA.generate(4096)

    with open('certs\\rsa_key', 'w') as f:
        priv = key.exportKey(format='PEM').decode('utf-8')
        f.write(priv)

    with open('certs\\rsa_key.pub', 'w') as f:
        pub = key.publickey().exportKey(format='PEM').decode('utf-8')
        f.write(pub)

if __name__ == '__main__':
    main()




