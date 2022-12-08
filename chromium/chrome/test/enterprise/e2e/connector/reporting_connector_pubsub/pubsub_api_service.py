# Copyright 2022 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json

from google.cloud import pubsub_v1


class PubsubApiService(object):
  """This class handles retrieving and verifying pubsub messages"""
  project = ''
  subscription = ''
  messages = None

  def __init__(self, credentials):
    """Construct a PubsubApiService instance.

    Args:
      credentials: A json string containing the project and
        pubsub subscription names.
    """
    credentialsJson = {}
    try:
      credentialsJson = json.loads(credentials)
    except json.JSONDecodeError:
      print('Decoding PubsubApiService credentials JSON has failed')
    self.project = credentialsJson['project']
    self.subscription = credentialsJson['subscription']

  def doesEventExist(self, deviceId, eventName):
    """Verifies if a specific message was sent. Lazy loads messages

    Args:
      deviceId: A GUID device id that made the action.
      eventName: A string containing the event name we're looking for
    """
    if self.messages is None:
      self.loadEvents()
    for msg in self.messages:
      msdData = msg
      if deviceId in msdData and eventName in msdData:
        return True
    return False

  def loadEvents(self):
    with pubsub_v1.SubscriberClient() as subscriber:
      subscription_path = subscriber.subscription_path(self.project,
                                                       self.subscription)
      response = subscriber.pull(request={
          "subscription": subscription_path,
          "max_messages": 500,
      })
      print('Loaded messages :' + str(len(response.received_messages)))
      ack_ids = [msg.ack_id for msg in response.received_messages]
      self.messages = [
          msg.message.data.decode() for msg in response.received_messages
      ]
      subscriber.acknowledge(request={
          "subscription": subscription_path,
          "ack_ids": ack_ids,
      })
