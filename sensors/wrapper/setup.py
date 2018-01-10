from setuptools import setup

setup(
    # This is the name of your project. The first time you publish this
    # package, this name will be registered for you. It will determine how
    # users can install this project, e.g.:
    #
    # $ pip install sampleproject
    #
    # And where it will live on PyPI: https://pypi.org/project/sampleproject/
    #
    # There are some restrictions on what makes a valid project name
    # specification here:
    # https://packaging.python.org/specifications/core-metadata/#name
    name='sensorwrapper',  # Required

    # Versions should comply with PEP 440:
    # https://www.python.org/dev/peps/pep-0440/
    #
    # For a discussion on single-sourcing the version across setup.py and the
    # project code, see
    # https://packaging.python.org/en/latest/single_source_version.html
    version='1.0.0',  # Required

    # This is a one-line description or tagline of what your project does. This
    # corresponds to the "Summary" metadata field:
    # https://packaging.python.org/specifications/core-metadata/#summary
    description='Sensor wrapper for the Savior system',  # Required

    # This should be a valid link to your project's main homepage.
    #
    # This field corresponds to the "Home-Page" metadata field:
    # https://packaging.python.org/specifications/core-metadata/#home-page-optional
    url='https://github.com/twosixlabs/savior/tree/master/sensors/wrapper',  # Optional

    # This should be your name or the name of the organization which owns the
    # project.
    author='Patrick Dwyer / Two Six Labs',  # Optional

    # This should be a valid email address corresponding to the author listed
    # above.
    author_email='patrick.dwyer@twosixlabs.com',  # Optional

    # Classifiers help users find your project by categorizing it.
    #
    # For a list of valid classifiers, see
    # https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[  # Optional
        # How mature is this project? Common values are
        #   3 - Alpha
        #   4 - Beta
        #   5 - Production/Stable
        'Development Status :: 3 - Alpha',

        # Indicate who your project is intended for
        'Intended Audience :: Developers',

        # Specify the Python versions you support here. In particular, ensure
        # that you indicate whether you support Python 2, Python 3 or both.
        'Programming Language :: Python :: 3.6',
    ],

    # You can just specify package directories manually here if your project is
    # simple. Or you can use find_packages().
    #
    # Alternatively, if you just want to distribute a single Python file, use
    # the `py_modules` argument instead as follows, which will expect a file
    # called `my_module.py` to exist:
    #
    #   py_modules=["my_module"],
    #
    py_modules=["sensor_wrapper"],

    # This field lists other packages that your project depends on to run.
    # Any package you put here will be installed by pip when your project is
    # installed, so they must be valid existing projects.
    #
    # For an analysis of "install_requires" vs pip's requirements files see:
    # https://packaging.python.org/en/latest/requirements.html
    install_requires=[
        'async-generator==1.8',
        'certifi==2017.11.5',
        'chardet==3.0.4',
        'curequests==0.4.0',
        'curio==0.8',
        'h11==0.7.0',
        'httptools==0.0.9',
        'idna==2.6',
        'kafka-python==1.3.5',
        'multidict==3.3.2',
        'Naked==0.1.31',
        'namedlist==1.7',
        'pycrypto==2.6.1',
        'PyYAML==3.12',
        'repoze.lru==0.7',
        'requests==2.18.4',
        'Routes==2.4.1',
        'shellescape==3.4.1',
        'six==1.11.0',
        'urllib3==1.22',
        'yarl==0.16.0',
    ],  # Optional

)