pipeline {
  agent any
  stages {
    stage('pull') {
      steps {
        sh 'echo "bl1_s5p4418 pipeline start"'
      }
    }
    stage('') {
      steps {
        setGerritReview()
        setGerritReview(customUrl: 'review.gerrithub.io', unsuccessfulMessage: 'abc')
      }
    }
  }
}