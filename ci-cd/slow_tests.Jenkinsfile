pipeline {
  agent {
    docker {
      image 'rikorose/gcc-cmake:latest'
      label 'qtr'
    }
  }
  stages {
    stage('build') {
      steps {
        sh '''
          cd cpp
          mkdir -p build && cd build
          cmake ..
          cmake --build . -j10
        '''
      }
    }
    stage('test') {
      steps {
        sh '''
          cd cpp/build
          ./bin/tests --gtest_output="xml:./report.xml"
        '''
        // publishHTML target: [
        //     allowMissing: false,
        //     alwaysLinkToLastBuild: false,
        //     keepAll: true,
        //     reportDir: 'cpp/build',
        //     reportFiles: 'report.html',
        //     reportName: 'Slow test report'
        //   ]
      }
    }
  }
  post {
        always{
            xunit (
                thresholds: [ skipped(failureThreshold: '0'), failed(failureThreshold: '0') ],
                tools: [ GoogleTest(pattern: 'cpp/build/report.xml') ]
            )
        }
    }
}