pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        sh '/bin/make'
      }
    }
    stage('error') {
      steps {
        archiveArtifacts 'adler32_cmp'
      }
    }
  }
}