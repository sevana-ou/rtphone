

#include "DummyRegistrationPersistenceManager.hxx"

using namespace b2bua;

void DummyRegistrationPersistenceManager::addAor(const resip::Uri& aor, const resip::ContactList contacts = resip::ContactList()) {
}

void DummyRegistrationPersistenceManager::removeAor(const resip::Uri& aor) {
}

bool DummyRegistrationPersistenceManager::aorIsRegistered(const resip::Uri& aor) {
  return false;
}

void DummyRegistrationPersistenceManager::lockRecord(const resip::Uri& aor) {
}

void DummyRegistrationPersistenceManager::unlockRecord(const resip::Uri& aor) {
}

resip::RegistrationPersistenceManager::UriList DummyRegistrationPersistenceManager::getAors() {
  return resip::RegistrationPersistenceManager::UriList();
}

resip::RegistrationPersistenceManager::update_status_t DummyRegistrationPersistenceManager::updateContact(const resip::Uri& aor, const resip::Uri& contact, time_t expires, float q) {
  return CONTACT_CREATED;
}

void DummyRegistrationPersistenceManager::removeContact(const resip::Uri& aor, const resip::Uri& contact) {
}

resip::ContactList DummyRegistrationPersistenceManager::getContacts(const resip::Uri& aor) {
  return resip::ContactList();
}


/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

